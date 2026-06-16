import argparse
import sys
import yfinance as yf
import datetime
import csv

if __name__ == "__main__":
	# Argument parsing
	parser = argparse.ArgumentParser()
	parser.add_argument('-t', '--tickers', nargs='+', required=True, help='List of tickers to get historical data from.')
	parser.add_argument('-sd', '--start', nargs='+', required=True, help='Start date of ticker tracking.')
	parser.add_argument('-ed', '--end', nargs='+', required=False, help='End date of ticker tracking.')
	args=parser.parse_args()

	ticker_list = args.tickers
	start_date = args.start[0]
	end_date = args.end[0] if args.end else datetime.date.today().isoformat()

	data = yf.download(ticker_list, start=start_date, end=end_date, group_by='tickers')

	# Using a CSV writer to write to stdout so the parent C++ script can obtain a CSV
	writer = csv.writer(sys.stdout)
	writer.writerows(data)
	sys.stdout.flush()
