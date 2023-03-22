package Checksums

import "crypto/md5"

func MD5Checksum(data []byte) string {
	hash := md5.New()
	hash.Write(data)
	return string(hash.Sum(nil))
}

func VerifyChecksum(data []byte, checksum string) bool {
	return checksum == MD5Checksum(data)
}
