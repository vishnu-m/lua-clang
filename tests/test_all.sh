for file in *.lua; do
        echo "Testing $file"
        busted $file
done