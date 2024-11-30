import sys

def write_to_file(n):
    with open('temp.tbl.csv', 'w') as f:
        for i in range(n):
            f.write(f"{i}|{i}|\n")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python generate_temp_tbl_csv.py <n>")
    else:
        try:
            n = int(sys.argv[1])
            write_to_file(n)
            print(f"Wrote {n} rows to 'temp.tbl.csv'.")
        except ValueError:
            print("Please provide a valid integer for n.")
