import os

CNN = ['yolo_tiny_in_im2col', 'yolo_tiny_out_im2col', 'alexnet_in_im2col', 'alexnet_out_im2col', 'Resnet50_in_im2col', 'Resnet50_out_im2col']
RNN = ['selfishrnn', 'DeepSpeech2_in_im2col', 'DeepSpeech2_out_im2col']
Recommendation = ['DLRM_in_im2col', 'DLRM_out_im2col', 'NCF']
Attention = ['gpt2']
test = ['test1_network', 'test2_network', 'test3_network', 'test4_network', 'test5_network', 'test6_network', 'test7_network']

benchmark_list = [CNN, RNN, Recommendation, Attention, test]
net_dir_list =['network_config/network_architecture/CNN/', 'network_config/network_architecture/RNN/', 'network_config/network_architecture/Recommendation/', 'network_config/network_architecture/Attention/', 'network_config/network_architecture/test/']

model_list = CNN + RNN + Recommendation + Attention + test

# Single netconfig_list
os.system("mkdir -p netconfig_list/single")
for i in range(len(benchmark_list)):
    for model in benchmark_list[i]:
        f = open("netconfig_list/single/"+model+".txt", 'w')
        f.write(net_dir_list[i]+model+".csv\n")
        f.close()

# Dual netconfig_list
os.system("mkdir -p netconfig_list/dual")
for i in range(len(benchmark_list)):
    for model in benchmark_list[i]:
        for j in range(len(benchmark_list)):
            for model2 in benchmark_list[j]:
                f = open("netconfig_list/dual/"+model+"_"+model2+".txt", 'w')
                f.write(net_dir_list[i]+model+".csv\n")
                f.write(net_dir_list[j]+model2+".csv\n")
                f.close()

# Quad netconfig_list
os.system("mkdir -p netconfig_list/quadruple")
for i in range(len(benchmark_list)):
    for model in benchmark_list[i]:
        for j in range(len(benchmark_list)):
            for model2 in benchmark_list[j]:
                for k in range(len(benchmark_list)):
                    for model3 in benchmark_list[k]:
                        for l in range(len(benchmark_list)):
                            for model4 in benchmark_list[l]:
                                f = open("netconfig_list/quadruple/"+model+"_"+model2+"_"+model3+"_"+model4+".txt", 'w')
                                f.write(net_dir_list[i]+model+".csv\n")
                                f.write(net_dir_list[j]+model2+".csv\n")
                                f.write(net_dir_list[k]+model3+".csv\n")
                                f.write(net_dir_list[l]+model4+".csv")
                                f.close()
