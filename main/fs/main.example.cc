
// See https://www.programming-electronics-diy.xyz/2024/01/defining-fcpu.html
#ifndef F_CPU
	#warning	"F_CPU not defined. Define it in project properties."
#elif F_CPU != 16000000
	#warning	"Wrong F_CPU frequency!"
#endif

#include "uart.h"
#include "fat.h"


int main(void){
	DIR dir;	// directory object
	FILE file;	// file object
	uint8_t return_code = 0;
	uint16_t dirItems = 0;
	
	_delay_ms(4000); // time to start the serial terminal
	
	// Print information on a serial terminal (optional)
	UART_begin(&uart0, 115200, UART_ASYNC, UART_NO_PARITY, UART_8_BIT);
	
	// Mount the memory card
	return_code = FAT_mountVolume();

	// If no error
	if(return_code == MR_OK){
		UART_sendString(&uart0, "Card mounted.\n");
		
		// Read label and serial number
		char label[12];
		char vol_sn_byte[4];
		uint32_t vol_sn = 0;
		FAT_getLabel(label, &vol_sn);
		
		// Extract serial number
		vol_sn_byte[0] = (vol_sn >> 24) & 0xFF;
		vol_sn_byte[1] = (vol_sn >> 16) & 0xFF;
		vol_sn_byte[2] = (vol_sn >> 8) & 0xFF;
		vol_sn_byte[3] = vol_sn & 0xFF;
		
		// Print serial number in hexadecimal format to serial terminal
		UART_sendString(&uart0, "\nVolume Serial Number is ");
		UART_sendHex8(&uart0, vol_sn_byte[0]);
		UART_sendHex8(&uart0, vol_sn_byte[1]);
		UART_sendString(&uart0, "-");
		UART_sendHex8(&uart0, vol_sn_byte[2]);
		UART_sendHex8(&uart0, vol_sn_byte[3]);
		UART_sendString(&uart0, "\n"); // new line character
		
		// Print label
		UART_sendString(&uart0, "Volume label is: ");
		UART_sendString(&uart0, label);
		UART_sendString(&uart0, "\n\n"); // 2x new line character
		
		
		// Make a directory in the root folder
		return_code = FAT_makeDir(&dir, "Logs Folder");
		
		if(return_code == FR_OK){
			UART_sendString(&uart0, "Folder created.\n");
		}else{
			UART_sendString(&uart0, "Folder could not be created.\n");
			UART_sendString(&uart0, "Return error code: ");
			UART_sendInt(&uart0, return_code);
			UART_sendString(&uart0, "\n");
		}
		
		// Open the created folder
		return_code = FAT_openDir(&dir, "Logs Folder");
		
		if(return_code == FR_OK){
			UART_sendString(&uart0, "Folder open: ");
			UART_sendString(&uart0, FAT_getFilename());
			UART_sendString(&uart0, "\n\n");
			
			// Create a new file in the currently open folder
			return_code = FAT_makeFile(&dir, "log_1.txt");
			
			if(return_code == FR_OK){
				UART_sendString(&uart0, "File created.\n\n");
			}else{
				UART_sendString(&uart0, "File could not be created.\n");
				UART_sendString(&uart0, "Return error code: ");
				UART_sendInt(&uart0, return_code);
				UART_sendString(&uart0, "\n\n");
			}
			
			
			// Get number of folders and files inside the directory
			dirItems = FAT_dirCountItems(&dir);
			UART_sendString(&uart0, "Folder has ");
			UART_sendInt(&uart0, dirItems);
			UART_sendString(&uart0, " items:\n");
			
			// Print folder content
			for(uint16_t i = 0; i < dirItems; i++){
				return_code = FAT_findNext(&dir, &file);
				
				if(FAT_attrIsFolder(&file)){
					UART_sendString(&uart0, "D, "); // Directory
				}else{
					UART_sendString(&uart0, "F, "); // File
				}
				
				UART_sendString(&uart0, "Idx: ");
				UART_sendInt(&uart0, FAT_getItemIndex(&dir));
				UART_sendString(&uart0, ", ");
				
				UART_sendString(&uart0, FAT_getFilename());
				UART_sendString(&uart0, "\n");
			}
		}else{
			UART_sendString(&uart0, "Could not open folder.\n");
			UART_sendString(&uart0, "Return code: ");
			UART_sendInt(&uart0, return_code);
			UART_sendString(&uart0, "\n\n");
		}

		
		/*FAT_openDir(&dir, "/");
		return_code = FAT_fdeleteByName(&dir, "Logging dir");
		UART_sendString(&uart0, "\nReturn code delete: ");
		UART_sendInt(&uart0, return_code);
		UART_sendString(&uart0, "\n");*/
			
		// Open a file for reading or writing
		// Open the folder containing the file
		FAT_openDir(&dir, "Logs Folder");
		return_code = FAT_fopen(&dir, &file, "log_1.txt");
			
		if(return_code == FR_OK){
			UART_sendString(&uart0, "\nFile open: ");
			UART_sendString(&uart0, FAT_getFilename());
			UART_sendString(&uart0, "\n\n");
			
			UART_sendString(&uart0, "File size: ");
			UART_sendInt(&uart0, FAT_getFileSize(&file));
			UART_sendString(&uart0, "\n");
			
			// Keep only first 10 bytes of the file (example)
			//FAT_fseek(&file, 10);
			//FAT_ftruncate(&file);
			
			// Move the writing pointer to the end of the file
			FAT_fseekEnd(&file);
			
			// Write a string
			FAT_fwriteString(&file, "Logging Date: 2024\n");
			
			// Write sensor output
			FAT_fwriteFloat(&file, 120.033, 3);
			FAT_fwriteString(&file, ",");
			FAT_fwriteFloat(&file, -0.221, 3);
			FAT_fwriteString(&file, ",");
			FAT_fwriteFloat(&file, -30.004, 3);
			FAT_fwriteString(&file, ",");
			FAT_fwriteFloat(&file, 0.023, 3);
			
			// Synchronize the writing buffer with the card
			FAT_fsync(&file);
			
			UART_sendString(&uart0, "File new size: ");
			UART_sendInt(&uart0, FAT_getFileSize(&file));
			UART_sendString(&uart0, " bytes.\n");
			
		}else if(return_code == FR_NOT_FOUND){
			// Make the file if it doesn't exist
			// ... code ...
			
			UART_sendString(&uart0, "File could not be found.\n");
			UART_sendString(&uart0, "Return code: ");
			UART_sendInt(&uart0, return_code);
			UART_sendString(&uart0, "\n\n");
		}else{
			UART_sendString(&uart0, "File could not be opened.\n");
			UART_sendString(&uart0, "Return code: ");
			UART_sendInt(&uart0, return_code);
			UART_sendString(&uart0, "\n\n");
		}
			
	}else{ // end if(return_code == MR_OK)
		UART_sendString(&uart0, "Card not mounted.");
		UART_sendString(&uart0, " Return code: ");
		UART_sendInt(&uart0, return_code);
		UART_sendString(&uart0, "\n\n");
	}
	
	

    while (1){
		_delay_ms(1000);
		
		if(sd_detected()){
			//UART_sendString(&uart0, "Card connected\n");	
		}else{
			//UART_sendString(&uart0, "Card disconnected\n");		
		}
    }
}