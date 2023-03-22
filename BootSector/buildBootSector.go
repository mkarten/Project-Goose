package BootSector

import (
	"fmt"
	"os"
)

func CreateBootSectorFile() {
	// Build the boot sector for the operating system and write it to a file
	binFile, err := os.OpenFile("boot.bin", os.O_CREATE|os.O_RDWR, 0644)
	if err != nil {
		panic(err)
	}
	defer binFile.Close()
	fmt.Println("Initializing boot sector...")
	instructions := InitializeBootSector()
	// TODO: Add assembly instruction to use during boot
	// Open the instructions binary file
	instructionsFile, err := os.OpenFile("instructions.bin", os.O_RDONLY, 0644)
	if err != nil {
		panic(err)
	}
	defer instructionsFile.Close()
	// Check the size of the instructions file
	instructionsFileInfo, err := instructionsFile.Stat()
	if err != nil {
		panic(err)
	}
	instructionsFileSize := instructionsFileInfo.Size()
	if instructionsFileSize > 510 {
		panic("Instructions file is too large! Aborting...")
	}
	// Read the instructions file into the boot sector
	fmt.Println(instructionsFileSize)
	instructionsFile.Read(instructions[:instructionsFileSize])

	// Creating the boot sector identifier
	instructions[510] = 0x55
	instructions[511] = 0xAA

	// Write the boot sector to the file
	binFile.Write(instructions[:])
	fmt.Println("Boot sector successfully created!")
}

func InitializeBootSector() []byte {
	bootSector := make([]byte, 512)
	for i := 0; i < 512; i++ {
		bootSector[i] = 0x90
	}
	return bootSector
}
