'''
1 v 1: calculate the average latency
1 v N: 1. get the max latency for each data transmission among the clients 2. calculate the average latency
'''
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

class MiddlewareAnalysis:
    def __init__(self, datanum, sample_time, clientnum, payload_lst):
        self.datanum = datanum
        self.sample_time = sample_time
        self.c_num = clientnum    # C
        self.payload_lst = payload_lst  # P
        self.latency = np.zeros(len(self.payload_lst))   # P
        self.latency_lst = np.zeros((self.datanum, len(self.payload_lst)))   # DATANUM x P
        
    def help_cal_latency(self, payload):
        # Specify the CSV file path
        csv_file_path = '../output/client-' + str(self.c_num) + '_payload-' + str(payload) + '_fre-' + str(self.sample_time) + '.csv'
        print(f'-------- Start {csv_file_path} ---------')

        # Read the CSV file into a DataFrame
        latency_df = pd.read_csv(csv_file_path)
        print('-------input latency----------')
        print(latency_df)

        # find the max latency among the clients
        max_latency_df = latency_df.max(axis=1)
        print('--------max latency among clients-------')
        print(max_latency_df)

        # Calculate the average of the maximum values
        average_latency = max_latency_df.mean()
        print(average_latency)
        
        return np.array(max_latency_df), average_latency
    
    def cal_latency(self):
        for i, payload in enumerate(self.payload_lst):
            max_lst, avg_val = self.help_cal_latency(payload)
            self.latency[i] = avg_val
            self.latency_lst[:, i] = max_lst
    
    def visualization(self):
        plt.plot(self.payload_lst, self.latency)    # (X, Y)
        locs, labels = plt.xticks()
        plt.boxplot(self.latency_lst, positions=self.payload_lst)
        plt.xticks(locs, locs)
        plt.xlabel('PayLoad Size')
        plt.ylabel('Avg. Latency (us)')
        plt.title(f'Latency Plot: 1 v {self.c_num} clients, sample_time={self.sample_time}')
        plt.show()

if __name__ == "__main__":
    c_num = 1
    payload_lst = [8, 80, 200, 500, 1000, 2000]
    sample_time = 1
    datanum = 500
    mqtt = MiddlewareAnalysis(datanum, sample_time, c_num, payload_lst)
    mqtt.cal_latency()
    mqtt.visualization()
    
    