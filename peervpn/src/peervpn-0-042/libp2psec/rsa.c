/***************************************************************************
 *   Copyright (C) 2013 by Tobias Volk                                     *
 *   mail@tobiasvolk.de                                                    *
 *                                                                         *
 *   This program is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/


#ifndef F_RSA_C
#define F_RSA_C


#include "crypto.c"
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bn.h>


// Minimum size of DER encoded RSA public key in bytes.
#define rsa_MINSIZE 48


// Maximum size of DER encoded RSA public key in bytes.
#define rsa_MAXSIZE 416


// The RSA structure.
struct s_rsa {
	int isvalid;
	int isprivate;
	EVP_PKEY *key;
	EVP_MD_CTX *md;
	BIGNUM *bn;
};


// Returns 1 if RSA structure contains a valid public key
static int rsaIsValid(const struct s_rsa *rsa) {
	return rsa->isvalid;
}


// Returns 1 if RSA structure contains a private key
static int rsaIsPrivate(const struct s_rsa *rsa) {
	return rsa->isprivate;
}


// Get size of DER encoded public key.
static int rsaGetDERSize(const struct s_rsa *rsa) {
	int len = i2d_PublicKey(rsa->key, NULL);
	if(len > 0) {
		return len;
	}
	else {
		return 0;
	}	
}


// Get DER encoded public key. Returns length if successful.
static int rsaGetDER(unsigned char *buf, const int buf_size, const struct s_rsa *rsa) {
	unsigned char *i2dbuf;
	int len;
	int ptlen = rsaGetDERSize(rsa);
	i2dbuf = buf;
	if((ptlen > rsa_MINSIZE) && (ptlen < buf_size)) {
		len = i2d_PublicKey(rsa->key, &i2dbuf);
		if(len > 0) {
			return len;
		}
		else {
			return 0;
		}
	}
	else {
		return 0;
	}	
}


// Get SHA-256 fingerprint of public key
static int rsaGetFingerprint(unsigned char *buf, const int buf_size, const struct s_rsa *rsa) {
	unsigned char derbuf[rsa_MAXSIZE];
	int dersize = rsaGetDER(derbuf, rsa_MAXSIZE, rsa);
	if(dersize > 0) {
		return cryptoCalculateSHA256(buf, buf_size, derbuf, dersize);
	}
	else {				
		return 0;
	}
}


// Generate RSA public/private key pair
static int rsaGenerate(struct s_rsa *rsa, const int key_size) {
	RSA *rsakey;
	rsa->isvalid = 0;
	if(key_size > 0) {
		rsakey = RSA_new();
		if(rsakey != NULL) {
			if(BN_set_word(rsa->bn, RSA_F4)) {
				if(RSA_generate_key_ex(rsakey, key_size, rsa->bn, NULL)) {
					if(RSA_check_key(rsakey) == 1) {
						if(EVP_PKEY_assign_RSA(rsa->key, rsakey)) {
							rsa->isvalid = 1;
							rsa->isprivate = 1;
							return 1;
						}
					}
				}
				BN_zero(rsa->bn);
			}
			RSA_free(rsakey);
		}
	}
	return 0;
}


// Load DER encoded public key.
static int rsaLoadDER(struct s_rsa *rsa, const unsigned char *pubkey, const int pubkey_size) {
	EVP_PKEY *d2ipkey;
	const unsigned char *d2ikey;
	rsa->isvalid = 0;
	if((pubkey_size > rsa_MINSIZE) && (pubkey != NULL)) {
		d2ikey = pubkey;
		d2ipkey = rsa->key;
		if(d2i_PublicKey(EVP_PKEY_RSA, &d2ipkey, &d2ikey, pubkey_size) != NULL) {
			rsa->isvalid = 1;
			rsa->isprivate = 0;
			return 1;
		}
		else {
			return 0;
		}
	}
	else {
		return 0;
	}
}


// Load PEM encoded public key.
static int rsaLoadPEM(struct s_rsa *rsa, unsigned char *pubkey, const int pubkey_size) {
	BIO *biopub;
	RSA *rsakey;
	int ret;
	rsa->isvalid = 0;
	if((pubkey_size > 0) && (pubkey != NULL)) {
		ret = 0;
		biopub = BIO_new_mem_buf(pubkey, pubkey_size);
		if(biopub != NULL) {
			rsakey = RSA_new();
			if(rsakey != NULL) {
				if(PEM_read_bio_RSA_PUBKEY(biopub, &rsakey, NULL, NULL) != NULL) {
					EVP_PKEY_assign_RSA(rsa->key, rsakey);
					rsa->isvalid = 1;
					rsa->isprivate = 0;
					ret = 1;
				}
				else {
					RSA_free(rsakey);
				}
			}
			BIO_free(biopub);
		}
		return ret;
	}
	else {
		return 0;
	}
}


// Load PEM encoded private key.
static int rsaLoadPrivatePEM(struct s_rsa *rsa, unsigned char *privkey, const int privkey_size) {
	BIO *biopriv;
	RSA *rsakey;
	int ret;
	rsa->isvalid = 0;
	if((privkey_size > 0) && (privkey != NULL)) {
		ret = 0;
		biopriv = BIO_new_mem_buf(privkey, privkey_size);
		if(biopriv != NULL) {
			rsakey = RSA_new();
			if(rsakey != NULL) {
				if(PEM_read_bio_RSAPrivateKey(biopriv, &rsakey, NULL, NULL) != NULL) {
					if(RSA_check_key(rsakey) == 1) {
						EVP_PKEY_assign_RSA(rsa->key, rsakey);
						rsa->isvalid = 1;
						rsa->isprivate = 1;
						ret = 1;
					}
					else {
						RSA_free(rsakey);
					}
				}
				else {
					RSA_free(rsakey);
				}
			}
			BIO_free(biopriv);
		}
		return ret;
	}
	else {
		return 0;
	}
}


// Return maximum size of a signature.
static int rsaSignSize(const struct s_rsa *rsa) {
	return EVP_PKEY_size(rsa->key);
}


// Generate signature. Returns length of signature if successful.
static int rsaSign(struct s_rsa *rsa, unsigned char *sign_buf, const int sign_len, const unsigned char *in_buf, const int in_len) {
	int sign_maxlen = rsaSignSize(rsa);
	int len;
	if(sign_len < sign_maxlen) return 0;
	if(!EVP_SignInit_ex(rsa->md, EVP_sha256(), NULL)) return 0;
	if(!EVP_SignUpdate(rsa->md, in_buf, in_len)) return 0;
	if(!EVP_SignFinal(rsa->md, sign_buf, (unsigned int *)&len, rsa->key)) return 0;
	if(!(len > 0)) return 0;
	return len;
}


// Verify signature. Returns 1 if successful.
static int rsaVerify(struct s_rsa *rsa, const unsigned char *sign_buf, const int sign_len, const unsigned char *in_buf, const int in_len) {
	if(!EVP_VerifyInit_ex(rsa->md, EVP_sha256(), NULL)) return 0;
	if(!EVP_VerifyUpdate(rsa->md, in_buf, in_len)) return 0;
	if(!EVP_VerifyFinal(rsa->md, sign_buf, sign_len, rsa->key)) return 0;
	return 1;
}


// Reset a RSA object.
static void rsaReset(struct s_rsa *rsa) {
	rsa->isvalid = 0;
	rsa->isprivate = 0;
}


// Create a RSA object.
static int rsaCreate(struct s_rsa *rsa) {
	rsa->bn = BN_new();
	if(rsa->bn != NULL) {
		BN_zero(rsa->bn);
		rsa->key = EVP_PKEY_new();
		if(rsa->key != NULL) {
			rsa->md = EVP_MD_CTX_create();
			if(rsa->md != NULL) {
				rsaReset(rsa);
				return 1;
			}
			EVP_PKEY_free(rsa->key);
		}
		BN_free(rsa->bn);
	}
	return 0;
}


// Destroy a RSA object.
static void rsaDestroy(struct s_rsa *rsa) {
	rsaReset(rsa);
	EVP_MD_CTX_destroy(rsa->md);
	EVP_PKEY_free(rsa->key);
	BN_free(rsa->bn);
}


#endif // F_RSA_C
