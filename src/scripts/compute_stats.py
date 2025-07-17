import sys
import csv

csv_reader = csv.reader(sys.stdin)

print("--------------------------------------------------------------------------------")
elements = {}
for row in csv_reader:
    print(",".join(row))
    tag = row[0]
    time = row[1]
    if tag in elements:
        elements[tag].append(int(time))
    else:
        elements[tag] = [int(time)]

print("--------------------------------------------------------------------------------")
print(",".join(["Tag", "N", "Min", "Median", "Max"]))
for key in elements:
    elements[key].sort()
    n = len(elements[key])
    print(",".join([key, str(n), str(min(elements[key])), str(elements[key][n // 2]), str(max(elements[key]))]));
