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
	kernelFile, err := os.OpenFile("kernel.bin", os.O_RDONLY, 0644)
	if err != nil {
		panic(err)
	}
	defer kernelFile.Close()
	fmt.Println("Initializing boot sector...")
	instructions := make([]byte, 5000)

	// TODO: Add assembly instruction to use during boot
	// Open the instructions binary file
	instructionsFile, err := os.OpenFile("instructions.bin", os.O_RDONLY, 0644)
	if err != nil {
		panic(err)
	}
	defer instructionsFile.Close()

	// Get the size of the instructions file
	instructionsFileInfo, err := instructionsFile.Stat()
	if err != nil {
		panic(err)
	}
	instructionsFileSize := instructionsFileInfo.Size()

	// Read the instructions file into the boot sector
	fmt.Println(instructionsFileSize)
	instructionsFile.Read(instructions[:instructionsFileSize])
	// Read the kernel file into the boot sector
	kernelFile.Read(instructions[512:])
	// Write the boot sector to the file
	binFile.Write(instructions[:])
	// Extend the file to 1.44 MB
	binFile.Seek(1474559, 0)
	binFile.Write([]byte{0x00})
	binFileStats, err := binFile.Stat()
	if err != nil {
		panic(err)
	}
	fmt.Println(binFileStats.Size())

	fmt.Println("Boot sector successfully created!")
}
