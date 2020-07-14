input_folder=input_objs
output_folder=output_objs
find "${input_folder}" -name "*.obj" | sed 's!.*/!!' | sort |
while IFS= read -r filename; do
echo "Reordering ${filename}"
cat "${input_folder}/${filename}" | grep 'v ' > "${output_folder}/${filename}"
cat "${input_folder}/${filename}" | grep 'vt ' >> "${output_folder}/${filename}"
cat "${input_folder}/${filename}" | grep 'f ' >> "${output_folder}/${filename}"
done
