void _cdecl ctest_kernel(){
    char *vidptr = (char*)0xb8000;  //video mem begins here.
    unsigned int i = 0;
    unsigned int j = 0;
    //clear screen
    while(j < 80 * 25 * 2) {
        //blank character
        vidptr[j] = ' ';
        //attribute-byte: light grey on black screen
        vidptr[j+1] = 0x07;
        j = j + 2;
    }
    j = 0;
    //this loop writes the string to video memory
    char kernel[] = "C Kernel";
    while(kernel[j] != '\0') {
        //the character's ascii
        vidptr[i] = kernel[j];
        //attribute-byte: light grey on black screen
        vidptr[i+1] = 0x07;
        ++j;
        i = i + 2;
    }
    return;
}