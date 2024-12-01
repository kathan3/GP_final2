import argparse
import random

def generate_queries(min_val, max_val, num_queries, overlapping):
    queries = []

    if overlapping:
        for _ in range(num_queries):
            x = random.randint(min_val, max_val - 1)
            y = random.randint(x + 1, max_val)
            queries.append((x, y))
    else:
        # Calculate the total available range
        total_range = max_val - min_val
        if total_range < num_queries:
            raise ValueError("Range too small for the number of non-overlapping queries requested.")

        # Determine equal-sized intervals
        interval_length = total_range // num_queries
        extra = total_range % num_queries

        current_start = min_val
        for i in range(num_queries):
            # Distribute any extra range to the intervals
            current_interval_length = interval_length + (1 if i < extra else 0)
            current_end = current_start + current_interval_length

            # Randomly adjust within the interval to get the query
            x = random.randint(current_start, current_end - 1)
            y = random.randint(x + 1, current_end)
            queries.append((x, y))

            current_start = current_end  # Move to the next interval

    return queries

def main():
    parser = argparse.ArgumentParser(description="Generate random queries.")
    parser.add_argument("min_val", type=int, help="Minimum value of the range.")
    parser.add_argument("max_val", type=int, help="Maximum value of the range.")
    parser.add_argument("num_queries", type=int, help="Number of queries to generate.")
    parser.add_argument("--overlapping", action="store_true", help="Generate overlapping queries.")
    parser.add_argument("--output_file", type=str, default="queries.txt", help="Output file name.")

    args = parser.parse_args()

    try:
        queries = generate_queries(args.min_val, args.max_val, args.num_queries, args.overlapping)
    except ValueError as e:
        print(f"Error: {e}")
        return

    # Write queries to the output file
    with open(args.output_file, "w") as file:
        for x, y in queries:
            file.write(f"{x} {y}\n")

    print(f"Generated {len(queries)} {'overlapping' if args.overlapping else 'non-overlapping'} queries and saved to '{args.output_file}'.")

if __name__ == "__main__":
    main()
