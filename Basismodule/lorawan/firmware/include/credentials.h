// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
#define LORA_APPEUI { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } // TTN Application EUI with "lsb"

// This should also be in little endian format, see above.  0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x04, 0xCF, 0xAF
#define LORA_DEVEUI { 0xEE, 0xCF, 0x04, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 } // TTN Device EUI with "lsb"

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
#define LORA_APPKEY { 0xAC, 0x21, 0xA0, 0xEE, 0x72, 0xA1, 0x0D, 0xD9, 0xC7, 0x88, 0xFC, 0x7F, 0x7F, 0x9E, 0xF4, 0x6C } // TTN App Key with "msb"