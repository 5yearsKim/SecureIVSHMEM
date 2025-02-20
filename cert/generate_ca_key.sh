#!/bin/bash

WORK_DIR=$(realpath $(dirname $0))
CA_DIR="$WORK_DIR/ca"


mkdir -p "$CA_DIR"

# Generate a 4096-bit RSA private key for the CA
echo "Generating CA private key"
openssl genrsa -out "$CA_DIR/ca.key" 4096

# Create a self-signed CA certificate valid for 10 years (3650 days)
# You will be prompted to enter details such as Country, Organization, and Common Name.
echo "Generating CA certificate"
openssl req -x509 -new -nodes -key "$CA_DIR/ca.key"\
    -sha256 -days 3650 -out "$CA_DIR/ca.crt"


# Create a directory to store the CA certificate
echo "Copying CA certificate to /tmp/ivshmem_ca"
mkdir -p /tmp/ivshmem_ca
cp "$CA_DIR/ca.crt" /tmp/ivshmem_ca

# # Certificate contains a public key. This command let you to extract the public key from a certificate.
# openssl x509 -in ca.crt -pubkey -noout

