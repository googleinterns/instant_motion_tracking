# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

input_folder=input_objs
output_folder=output_objs
find "${input_folder}" -name "*.obj" | sed 's!.*/!!' | sort |
while IFS= read -r filename; do
echo "Reordering ${filename}"
cat "${input_folder}/${filename}" | grep 'v ' > "${output_folder}/${filename}"
cat "${input_folder}/${filename}" | grep 'vt ' >> "${output_folder}/${filename}"
cat "${input_folder}/${filename}" | grep 'f ' >> "${output_folder}/${filename}"
done
