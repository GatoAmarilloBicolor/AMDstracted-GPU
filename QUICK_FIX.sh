#!/bin/bash
# One-line fix for Haiku users: gets the correct Build.sh from GitHub

cd ~/src/AMDstracted-GPU

echo "Updating from GitHub..."
git pull origin main

echo "Cleaning old builds..."
cd AMDGPU_Abstracted
rm -rf mesa_build builddir_mesa builddir_accelerant builddir_AMDGPU_Abstracted

echo "Done. Now run:"
echo "  ./Build.sh"
