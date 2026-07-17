一：Overview
	 This tool is a digital signature tool, including RSA and SM2 algorithms. Used to generate signatures for nationalchip bootloader and other files(such as kernel or app) .
 
二：How to use the signature tool
		<1>：Execute ./digital_signature.sh script to see how to use it and show all support options.
		
		<2>：In order to be able to modify the configuration files required for signature, you should read the following file description in detail.
			.
			├── chip_config						// chip acceleration configuration, users do not need to care, these configuration are valid only when option <bootloader> = 1.
			│  	├── reg_config_gx3113c.txt
			│  	├── reg_config_gx3211.txt
			│  	└── reg_config_gx6605s.txt
			├── common_config
			│   └── market_id.txt	            // Market id, need to be the same as the market id value in otp. Users need to care about this configuration, this configuration are valid only when option <bootloader> = 1.
			│								    // For example, the value of market id in otp is 0x524b414d, Then the content in the market_id.txt file shoud be 4d41524b.
			├── common_fun.sh                   // Internal shell function, users do not need to care.
			├── digital_signature.sh	        // Digital signature script.
			├── input							// Input directory, files that need to be signed need to be placed in this directory.
			├── output							// Output directory, the generated signature file is placed in this directory.	
			├── readme.txt
			├── rsa								// RSA related configuration, these configurations are only used when the option <sign_algo> = RSA_SMI or RSA_PKCS
			│   ├── config
			│   │   └── switch_mark_id.txt	    // The content of this file is the address and content of the switch market id on otp, only need to care about this configuration when the option <sign_algo>=RSA_SMI.
			│   │								// The first line of the file is the address of switch market id on otp，the second line of the file is the value at this address, which is the value of switch market id.
			│   │				                // For example, the address of switch market id on otp is 0x00000901, and the value from the low address to the high address is 11223344,
			│	│								// then the content of the first line in the switch_mark_id.txt file is 01090000, and the content of the second line is 11223344.
			│   ├── key							// Key used for signature
			│   │   ├── 1nd_key_modulus.bin     // Level 1 public key modulus, binary file, stored on otp, users do not need to care this file.
			│   │   ├── 1nd_key.pri				// Level 1 private key, user needs to replace the file content with his own private key.
			│   │   ├── 1nd_key.pub				// Level 1 public key, users do not need to care this file.
			│   │   ├── 2nd_key_modulus.bin		// Level 2 public key modulus, binary file, stored on flash, only need to care about this configuration when the option [secure_level] = 2.
			│   │   ├── 2nd_key.pri				// Level 2 private key, user needs to replace the file content with his own private key, only need to care about this configuration when the option [secure_level] = 2.
			│   │   └── 2nd_key.pub				// Level 2 public key, users do not need to care this file.
			│   ├── rsa_generate_key.sh         // RSA key generation script
			│   └── sign.sh						// RSA digital signature script.
			└── sm2								// SM2 related configuration, these configurations are only used when the option <sign_algo> = SM2
			    ├── config						// Elliptic curve equation parameters, the default is the recommended value, if you want to modify the parameters you need to modify these configuration files
				│   ├── a.txt					// The parameter a of the elliptic curve equation
				│   ├── b.txt					// The parameter b of the elliptic curve equation
				│   ├── Gx.txt					// The x coordinate of point G
				│   ├── Gy.txt					// The y coordinate of point G
				│   ├── IDA.txt					// A user's identifiable identifier, hex file
				│   ├── n.txt					// Base point G
				│   └── p.txt					// A prime field	
				├── create_r_s.c				// Used to convert the encrypted ciphertext into (r, s) form, users do not need to care.
				├── key							// Key used for signature
				│   ├── 1nd_key.pri				// Level 1 private key, the user needs to replace the file content with his own private key..
				│   ├── 1nd_key.pub				// Level 1 public key, users do not need to care this file.
				│   ├── 1nd_key_public_x.bin    // The x coordinate of the level 1 public key, binary file, stored on otp.
				│   ├── 1nd_key_public_y.bin    // The y coordinate of the level 1 public key, binary file, stored on otp.
				│   ├── 2nd_key.pri				// Level 2 private key, the user needs to replace the file content with his own private key, only need to care about this configuration when the option [secure_level] = 2.
				│   ├── 2nd_key.pub				// Level 2 public key, users do not need to care this file.
				│   ├── 2nd_key_public_x.bin	// The x coordinate of the level 2 public key, binary file, stored on flash, only need to care about this configuration when the option [secure_level] = 2.
				│   └── 2nd_key_public_y.bin	// The y coordinate of the level 2 public key, binary file, stored on flash, only need to care about this configuration when the option [secure_level] = 2.
				├── sign.sh						// SM2 digital signature script.
				└── sm2_generate_key.sh			// SM2 key generation script


三：How to use the key generation script
	<1>：RSA
			The rsa/rsa_generate_key.sh file is used to generate the RSA key pair.
			Execute ./rsa_generate_key.sh script to see how to use it and show all support options, as follows：	


	<2>：SM2
			The sm2/sm2_generate_key.sh file is used to generate the SM2 key pair.
			Execute ./sm2_generate_key.sh script to see how to use it and show all support options, as follows：	


四：Example
	<1>：Generate RSA_SMI level2 signature for sirius bootloader file(loader-sflash.bin)
		 step1：Copy file loader-sflash.bin to the input directory
		 step2：Modify the configuration related to RSA_SMI, such as market id and secret key, etc.
		 step3：Execute ./digital_signature.sh script，command is as follows：
				./digital_signature.sh 1 loader-sflash.bin RSA_SMI sirius 2
		 step4：The generated file is in the output directory，as follows：
			　	output/
			　	├── loader_level2_stage1_code.bin			// bootloader stage1 code
		　　　	├── loader_level2_stage2_code.bin			// bootloader stage2 code
			　	├── loader-sflash-level2.bin				// the resulting signature file	
			　	└── market_id.bin							// market id value in the common_config/market_id.txt file


	<2>：Generate SM2 signature for other file(test.bin)
		 step1：Copy file test.bin to the input directory
		 step2：Modify the configuration related to SM2, such as market id and secret key, etc.
		 step3：Execute ./digital_signature.sh script，command is as follows：
				./digital_signature.sh 0 test.bin SM2
		 step4：The generated file is in the output directory，as follows：
				output/
				├── test.bin-sign_r                         // The r format of the generated signature file
				└── test.bin-sign_s							// The s format of the generated signature file

	<3>：Generate RSA_SMI signature for other file(test.bin)
		 step1：Copy file test.bin to the input directory
		 step2：Modify the configuration related to RSA_SMI, such as market id and secret key, etc.
		 step3：Execute ./digital_signature.sh script，command is as follows：
				./digital_signature.sh 0 test.bin RSA_SMI
		 step4：The generated file is in the output directory，as follows：
				output/
				└── test.bin-sign                          // Generated signature file
