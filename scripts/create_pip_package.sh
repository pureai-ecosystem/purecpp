set -e
set -x

PACKAGE_DIR="/app/package"
BUILD_DIR="/app/build/Release"
SETUP_PY="$PACKAGE_DIR/setup.py"

rm -f "$PACKAGE_DIR/purecpp/"*.so
rm -rf "$PACKAGE_DIR/dist/"*

cp "$BUILD_DIR/"*.so "$PACKAGE_DIR/purecpp/"

CURRENT_VERSION=$(grep "version=" "$SETUP_PY" | sed -E "s/.*version=['\"]([0-9]+\.[0-9]+\.[0-9]+)['\"].*/\1/")
IFS='.' read -r MAJOR MINOR PATCH <<< "$CURRENT_VERSION"

NEW_VERSION="$MAJOR.$MINOR.$((PATCH + 1))"
sed -i "s/version=['\"]$CURRENT_VERSION['\"]/version=\"$NEW_VERSION\"/" "$SETUP_PY"

echo "update package version from $CURRENT_VERSION to $NEW_VERSION"

cd "$PACKAGE_DIR"

python3 setup.py bdist_wheel --python-tag cp312
python3 -m twine upload dist/*
