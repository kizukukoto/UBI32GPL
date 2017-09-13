USE_REFRESH=1
UPGRADE_PARTITION=/dev/mtd1

get_ubicom_magic_word() {
	get_image "$1" | dd bs=4 count=1 2>/dev/null | hexdump -C | awk '$2 { print $2 $3 $4 $5 }'
}

platform_check_image() {
	[ "$ARGC" -gt 1 ] && return 1

	case "$(get_ubicom_magic_word "$1")" in
		c90055aa) return 0;;
		*)
			echo "Invalid image type"
			return 1
		;;
	esac
}

platform_do_upgrade() {
	eraseall $UPGRADE_PARTITION
	echo "Writing new firmware to the device $UPGRADE_PARTITION"
	get_image "$1" > $UPGRADE_PARTITION
	reboot
}

