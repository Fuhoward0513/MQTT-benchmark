import os
import csv
import sys

class FILE:
    def __init__(self, N, M, st, payload):
        self.N = N
        self.M = M
        self.st = st
        self.payload = payload
        self.file_dir = "/root/mqtt/tmp/"
        self.combine_dir = "/root/mqtt/output/"
        
        self.file_names = None
        
    def get_file_names(self):
        self.file_names = [file for file in os.listdir(self.file_dir) if os.path.isfile(os.path.join(self.file_dir, file))]
        # print(self.file_names)

    def combine_files(self):
        # Read the contents of each input CSV file into a list of rows
        data_list = []
        for f in self.file_names:
            csv_file = self.file_dir + f
            data = []
            with open(csv_file, 'r') as file:
                csv_reader = csv.reader(file)
                for row in csv_reader:
                    data.append(row)
            data_list.append(data)

        # Combine the data from all files into a new list
        combined_data = []
        num_rows = len(data_list[0])
        for i in range(num_rows):
            combined_row = []
            for data in data_list:
                combined_row.extend(data[i])
            combined_data.append(combined_row)

        # Write the combined data to a new CSV file
        output_csv = self.combine_dir + "N=" + self.N + "_M=" + self.M + "_st=" + self.st + "_payload=" + self.payload + ".csv"
        with open(output_csv, 'w', newline='') as outfile:
            csv_writer = csv.writer(outfile)
            csv_writer.writerows(combined_data)
    
    def delete_file(self):
        for file_name in self.file_names:
            file_path = os.path.join(self.file_dir, file_name)
            if os.path.isfile(file_path):
                os.remove(file_path)
                print(f"Deleted: {file_path}")
            else:
                print(f"Skipped (not a file): {file_path}")
        


if __name__ == '__main__':
    if(len(sys.argv)!=5):
        print("<N>, <M>, <st>, <payload>")
    
    else:
        N, M, st, payload = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]
        f = FILE(N, M, st, payload)
        
        f.get_file_names()
        f.combine_files()
        f.delete_file()