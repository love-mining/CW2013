# This is 2013AI_cw3.py by Pengyu CHEN(cpy.prefers.you@gmail.com)
# As the course work of Artificial Intelligence, 2013.02-2013.04
# COPYLEFT, ALL WRONGS RESERVED.

# Original problem:
#   Make prediction of a stock by using the artificial neural network

import pyfann.libfann as libfann
import matplotlib.pyplot as plot

def main():
    # setting the prediction parameters 
    known_days = 7
    predict_days = 1
    verify_days = 30

    # setting up the parameters of the network
    connection_rate = 1
    learning_rate = 0.1
    num_input = known_days * 2
    num_hidden = 60
    num_output = predict_days
    
    # setting up the parameters of the network, continued
    desired_error = 0.000040
    max_iterations = 10000
    iteration_between_reports = 100

    # setting up the network
    net = libfann.neural_net()
    net.create_sparse_array(connection_rate, (num_input, num_hidden, num_output))
    net.set_learning_rate(learning_rate)
    net.set_activation_function_output(libfann.SIGMOID_SYMMETRIC_STEPWISE)

    # read the input file and format data
    fin = open("cw3.in")
    lines = fin.readlines()
    fin.close()
    rawdata = list(map(float, lines))[-1000:]
    datain0 = rawdata[0::2]
    datain1 = rawdata[1::2]
    n0 = max(datain0) * 1.4
    n1 = max(datain1) * 1.4
    datain0 = list(map(lambda x: x / n0, datain0))
    datain1 = list(map(lambda x: x / n1, datain1))

    # train the network
    data = libfann.training_data()
    drange = range(len(datain0) - known_days - verify_days)
    data.set_train_data(
        map(lambda x: datain0[x:][:known_days] + datain1[x:][:known_days], drange),
        map(lambda x: datain0[x + known_days:][:predict_days], drange)
        )
    net.train_on_data(data, max_iterations, iteration_between_reports, desired_error)

    # 
    result = []
    for i in range(verify_days):
        dtest = datain0[-known_days - verify_days + i:][:known_days] + datain1[-known_days - verify_days + i:][:known_days]
        result += [net.run(dtest)[0] * n0]
    plot.plot(list(map(lambda x: x * n0, datain0[-verify_days: -verify_days])) + result, "r")
    plot.plot(map(lambda x: x * n0, datain0[-verify_days:]), "b")
    #plot.plot(list(map(lambda x: x * n0, datain0[-verify_days * 2: -verify_days])) + result, "r")
    #plot.plot(map(lambda x: x * n0, datain0[-verify_days * 2:]), "b")
    plot.show()

#    net.train_on_file("cw3.in", max_iterations, iteration_between_reports, desired_error)
    #print(net.run([1,1]))
    print("hehe")
    return

if __name__ == "__main__":
    main()
