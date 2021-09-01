def csv_reader(file_name):
    for row in open(file_name, "r"):
        print("before yielding")
        yield row



print("Start reading");
csv_gen = csv_reader("../../tests/testresults/TestADAGUCFeatureFunctions/test_ADAGUCFeatureFunctions_testdata.csv")
row_count = 0

print("Start counting");
for row in csv_gen:
    print("Iterated one");
    row_count += 1

print(f"Row count is {row_count}")