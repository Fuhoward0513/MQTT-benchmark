#include <chrono>
#include <thread>
#include "include/linux-system-usage.hpp"

using namespace get_system_usage_linux;

int main(int argc, char * argv[]){
    std::ofstream myfile;
    // argv[1] payload, argv[2] sample_time
    // argv[3] n_sub, argv[4] m_pub
    // myfile.open(std::string("1v") + std::string(argv[3]) + "_" + std::string(argv[1]) + "_" + std::string(argv[2]) + "hz.csv");
    // int time = 2*505/std::stoi(argv[2])+1;
    int time = 2*505*std::stoi(argv[2])/1000+1;
    float usage[2][time];
    CPU_stats t1 = read_cpu_data();
    for(int i=0; i<time; i++){
        // CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        CPU_stats t2 = read_cpu_data();
        // std::cout << "CPU usage is " << (1000.0f * get_cpu_usage(t1, t2)) << "%\n";
        usage[0][i] = get_cpu_usage(t1, t2);

        // Memory usage
        auto memory_data = read_memory_data();
        // std::cout << "Memory usage is " << (memory_data.total_memory/1024/1024 * memory_data.get_memory_usage()) << "GB\n";
        usage[1][i] = memory_data.get_memory_usage();
        // myfile << 100.0f * get_cpu_usage(t1, t2) << ',' << (memory_data.total_memory/1024/1024 * memory_data.get_memory_usage()) << '\n';   
    }

    std::cout << "stats complete" << std::endl;
    // Write to file
    myfile.open("/root/mqtt/cpu_mem/N=" + std::string(argv[3]) + "_M=" + std::string(argv[4]) 
                + "_st=" + std::string(argv[2]) + "_payload=" + std::string(argv[1]) + ".csv");
    for(int i=0; i<time; i++){
        myfile << usage[0][i] << ',' << usage[1][i]-0.327913 << '\n';
    }
    myfile.close();
    
    return 0;
}