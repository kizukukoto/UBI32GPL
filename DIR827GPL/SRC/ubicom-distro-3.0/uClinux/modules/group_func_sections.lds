OUTPUT_ARCH(ubicom32)

SECTIONS {

	.init.text : {
		*(.init.text)
	}
	. = 0;

	.exit.text : {
		*(.exit.text)
	}
	. = 0;

	.ocm_text : {
		*(.ocm_text.*)
	}
	. = 0;

	.text : {
		*(.text.*)
	}
}
