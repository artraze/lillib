#!/bin/bash
# I rather dislike makefiles...
# Note that the PATH is required because otherwise "make" runs the one from MinGW and that doesn't mix well with the arm toolchain
export PATH="/bin:/usr/local/bin:/usr/bin:/cygdrive/c/Windows/SYSTEM32:/cygdrive/c/Windows"

LIBROOT="$(dirname "$(readlink -f "$0")")"
BUILDDIR="__builddir__"
SRCFILES="*.c *.cc"
INCDIRS="."
APPNAME="app"
OUT="$BUILDDIR/$APPNAME"
REBUILD=""
BOARD_CFG="board_cfg.h"

while (($#)); do
	case "$1" in
	"--rebuild") REBUILD="rebuild" ;;
	*) SRCFILES="$SRCFILES \"$1\"" ;;
	esac
	shift 1
done


function get_board_cfg()
{
	if ! sed -r "s/#define\\s+$1\\s+\"([^\"]*)\".*/\1/;tY;\$q1;d;:Y;q0" "$BOARD_CFG"
	then
		echo "Couldn't find $1"
		exit 1
	fi
}

BOARD_CPU=$(get_board_cfg "BOARD_CFG_CPU")
BOARD_STACK_SIZE=$(get_board_cfg "SYS_CFG_STACK_SIZE")

LD_SCRIPT="$(echo $BOARD_CPU | tr '[:upper:]' '[:lower:]').ld"

# Generate some additional info based on the CPU
case $BOARD_CPU in
	# Class
	ATMEGA328P*)
		CPUDEF="ATMEGA328P"
		LIBDIR="AVR"
		MFLAGS="-mmcu=atmega328p"
		;;&
	STM32F030*)
		CPUDEF="STM32F030"
		;;&
	STM32F051*)
		CPUDEF="STM32F051"
		;;&
	STM32F103*)
		CPUDEF="STM32F10X_MD"
		LD_SCRIPT="stm32f10x.ld"
		;;&
	# CPU arch
	STM32F0*)
		LIBDIR="STM32F0"
		MFLAGS="-mcpu=cortex-m0 -mthumb -mfloat-abi=soft"
		MCFLAGS=""
		;;&
	STM32F1*)
		LIBDIR="STM32F1"
		SRCFILES="$LIBROOT/$LIBDIR/stm32f10x/*.c $SRCFILES"
		MFLAGS="-mcpu=cortex-m3 -mthumb -mfloat-abi=soft"
		MCFLAGS=""
		;;&
	# Overall cpu type (which terminates the matches)
	STM32*)
		#export PATH="/cygdrive/c/SysGCC/arm-eabi/bin:$PATH"
		TOOLCHAIN="arm-none-eabi"
		MCFLAGS="$MCFLAGS -mthumb-interwork -falign-functions=8"
		LDFLAGS="-nostdlib -nodefaultlibs -nostartfiles -Wl,-nostdlib -Wl,--defsym=_stack_size=$BOARD_STACK_SIZE -Wl,-L,$LIBROOT/$LIBDIR -T $LIBROOT/$LIBDIR/$LD_SCRIPT"
		;;
	ATMEGA*)
		#export PATH="/cygdrive/c/SysGCC/avr/bin:$PATH"
		TOOLCHAIN="avr"
		MCFLAGS="-fmerge-all-constants -mbranch-cost=2 -fno-inline -fweb -fno-builtin-strcmp"
		LDFLAGS=""
		;;
	*)
		echo "Unknown uC '$1'"
		exit 1
		;;
esac

SRCFILES="$LIBROOT/$LIBDIR/*.c $LIBROOT/$LIBDIR/*.cc $LIBROOT/*.c $LIBROOT/widget/*.c $SRCFILES"
INCDIRS="$INCDIRS $LIBROOT/$LIBDIR $LIBROOT"
for i in $INCDIRS; do INCFLAGS="$INCFLAGS -I $i"; done
CFLAGS="$MFLAGS $MCFLAGS -Os -fdata-sections -ffunction-sections -Wall -fsigned-char -D $CPUDEF"
CCFLAGS="-std=gnu99"
CXXFLAGS="-std=c++17"

LDFLAGS="$MFLAGS -Wl,--gc-sections,--warn-section-align $LDFLAGS"

function should_build()
{
	local DEP
	local SRCFILE="$1"
	local OBJFILE="$2"
	local DEPFILE="${OBJFILE%.o}.d"
	if [ "$REBUILD" = "rebuild" ]; then return 0; fi
	if ! [ -f "$DEPFILE" ]; then echo "No $DEPFILE"; return 0; fi
	# Parsing the .d is a little troublesome since it escapes spaces in names with "\ ", which is
	# reasonable, but there doesn't seem to be an easy way to get bash to handle that when expanded
	# from a variable.  The line wrapping is also a pain.  So here sed is (ab)used to parse it.
	# The script does the following steps:
	#  - Trim start
	#  - Read in all lines
	#  - Convert line wraps (\<newline>) to newlines
	#  - Convert unescaped <space> to newline (at this point it's 1 file per line)
	#  - Remove backslash escapes (which appear to be used just for specific escapes
	#    i.e. "x\y.h" is escaped to "x\y.h" while "x\ y.h" is escaped to "x\\\ y.h"
	#    Also apparently "a$y.h" is escaped "a$$y.h" what is this?!
	local DEPS=$(sed -E '1s/.*:\s*//;:loop;/\\\s*$/{N;bloop};s/\s*\\\s*\n\s*/\n/g;s/([^\\]) /\1\n/g;s/\\([ \\])/\1/g;s/\$\$/$/g;q' "$DEPFILE")
	# To get that into bash, correctly handing special chars, switch IFS to use newline only and
	# process into an array
	IFS=$'\n'
	DEPFILES=($DEPS)
	IFS=$' \t\n'
	for DEP in "$BOARD_CFG" "$SRCFILE" "${DEPFILES[@]}"
	do
		if ! [ -f "$DEP" ]; then echo "ERROR: missing dep file $DEP"; fi
		if [ "$DEP" -nt "$OBJFILE" ]
		then
			return 0
		fi
	done
	return 1
}

# Compile
shopt -s nullglob
if ! SRCFILES=($SRCFILES); then exit 1; fi
shopt -u nullglob
OBJFILES=()
for SRCFILE in "${SRCFILES[@]}"
do
	OBJFILE="${SRCFILE//../}"
	OBJFILE="$BUILDDIR/obj/${OBJFILE%.*}.o"
	OBJFILES+=("$OBJFILE")
	#rm "$OBJFILE"
	if should_build "$SRCFILE" "$OBJFILE"
	then
		echo "$SRCFILE"
		EXT="${SRCFILE##*.}"
		mkdir -p $(dirname "$OBJFILE")
		if [ "$EXT" = "cc" ]
		then
			$TOOLCHAIN-g++ -save-temps=obj -MD -c $CFLAGS $CXXFLAGS $INCFLAGS "$SRCFILE" -o "$OBJFILE" || exit
		elif [ "$EXT" = "c" ]
		then
			$TOOLCHAIN-gcc -save-temps=obj -MD -c $CFLAGS $CCFLAGS $INCFLAGS "$SRCFILE" -o "$OBJFILE" || exit
		else
			echo "Unknown extension '$EXT' for source '$SRCFILE'"
			exit 1
		fi
	fi
done

echo "Linking...."
$TOOLCHAIN-gcc $LDFLAGS "${OBJFILES[@]}" -o "$OUT.elf" || exit
$TOOLCHAIN-objcopy --strip-debug "$OUT.elf" "$OUT.elf" || exit
$TOOLCHAIN-objcopy -O binary "$OUT.elf" "$OUT.bin" || exit
$TOOLCHAIN-objcopy -O ihex "$OUT.elf" "$OUT.hex" || exit
$TOOLCHAIN-objdump -s -x "$OUT.elf" > "$OUT.dump"
$TOOLCHAIN-objdump -d -x "$OUT.elf" > "$OUT.dasm"
$TOOLCHAIN-nm -a --format=sysv --size-sort --print-size --radix=x "$OUT.elf" | \
	sed -E 's/^([^|]*)\|/\1                                            |/;s/^([^|]{40}[^| ]*)\s*\|/\1|/' > "$OUT.syms"
echo "Image Size: $(stat -c %s $OUT.bin) bytes"
