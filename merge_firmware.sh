esptool.py --chip esp32s3 merge_bin -o build/merged-flash.bin 0x1000 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/cardputer_doom.bin
