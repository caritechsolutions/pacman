一：Overview
	This tool is a data encryption tool, including AES and SM4 algorithms. Used to encrypt nationalchip bootloader and other files(such as kernel or app) .

	
二：How to use the encryption tool
	<1>：Execute ./data_encrypt.sh script to see how to use it and show all support options.

	<2>：In order to be able to modify the configuration files required for encryption, you should read the following file description in detail.
			.
			├── config                      // configuration files required for encryption, such as key and vectors,
			│   │				// if you want to modify these configurations you need to modify the contents of these files.
			│   ├── iv.txt			// vectors
			│   ├── stage1_data_key.txt	// key used to encrypt stage1 data
			│   ├── stage1_derive_key.txt	// key used to encrypt stage1 data key, this key is key1, only <stage1_derive> = 1 this key is valid
			│   ├── stage2_data_key.txt	// key used to encrypt stage2 data
			│   └── stage2_derive_key.txt   // key used to encrypt stage2 data key, this key is key1, only <stage2_derive> = 1 this key is valid
			├── data_encrypt.sh             // encryption tool script
			├── input                       // input directory, the encrypted file needs to be placed in this directory
			├── output                      // output directory, the encrypted ciphertext is placed in this directory
			└── readme.txt
