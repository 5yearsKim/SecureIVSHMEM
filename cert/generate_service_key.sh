#!/bin/bash

WORK_DIR=$(realpath $(dirname $0))
CA_DIR="$WORK_DIR/ca"

# check argument and raise error if not provided
if [ -z "$1" ]; then
    echo "Usage: $0 <service_name>"
    exit 1
fi

SERVICE_NAME=$1
SERVICE_DIR="$WORK_DIR/$SERVICE_NAME"

mkdir -p "$SERVICE_DIR"

# Generate a 2048-bit RSA private key for the service
echo "Generating service private key"
openssl genrsa -out "$SERVICE_DIR/$SERVICE_NAME.key" 2048


# Create a certificate signing request (CSR) for the service
echo "Generating service certificate signing request"
openssl req -new -key "$SERVICE_DIR/$SERVICE_NAME.key" -out "$SERVICE_DIR/$SERVICE_NAME.csr"


# Sign the service certificate with the CA
echo "Signing service certificate with CA"
openssl x509 -req -in "$SERVICE_DIR/$SERVICE_NAME.csr" \
    -CA "$CA_DIR/ca.crt" -CAkey "$CA_DIR/ca.key" -CAcreateserial -out "$SERVICE_DIR/$SERVICE_NAME.crt" \
    -days 365 -sha256

# Verify the service certificate
echo "Service certificate generated at $SERVICE_DIR/$SERVICE_NAME.crt"





