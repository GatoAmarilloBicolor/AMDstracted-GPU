#!/bin/bash
if [ -z "$1" ]; then
  echo "Usage: ./upload_to_git.sh \"commit message\""
  exit 1
fi

echo "🚀 Adding all files..."
git add .

echo "📦 Committing with message: '$1'..."
git commit -m "$1"

echo "⬆️ Pushing to origin main..."
git push origin main

echo "✅ Done!"
