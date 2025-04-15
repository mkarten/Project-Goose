package BootSector

import (
	"fmt"
	"os"
	"strings"
)

func CreateBootSectorFile() {
	// Build the boot sector for the operating system and write it to a file
	binFile, err := os.OpenFile("boot.img", os.O_CREATE|os.O_RDWR, 0644)
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
	instructions := make([]byte, 100000)

	// TODO: Add assembly instruction to use during boot
	// Open the instructions binary file
	var bootloaderStages []*os.File
	// read all the .bin files in the bootloaderStages directory
	bootloaderStagesDir, err := os.ReadDir("bootloaderStages")
	if err != nil {
		panic(err)
	}
	for _, file := range bootloaderStagesDir {
		if file.IsDir() || !strings.HasSuffix(file.Name(), ".bin") {
			continue
		}
		fmt.Println("Reading bootloader stage: ", file.Name())
		bootloaderStage, err := os.OpenFile("bootloaderStages/"+file.Name(), os.O_RDONLY, 0644)
		if err != nil {
			panic(err)
		}
		bootloaderStages = append(bootloaderStages, bootloaderStage)
	}

	for _, stage := range bootloaderStages {
		stageStats, err := stage.Stat()
		if err != nil {
			panic(err)
		}
		stageSize := stageStats.Size()
		stage.Read(instructions[:stageSize])
		binFile.Write(instructions[:stageSize])
	}
	// Read the kernel file into the boot sector
	kernelFile.Read(instructions[:])
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
