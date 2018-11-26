import sys
import pandas as pd

input_args = list(sys.argv)

path = input_args[1]
output_filename = path + 'transients.txt'
pay_output = path + 'payment_output.csv'



with open (pay_output, 'rb') as input_file, open(output_filename, 'wb') as output_file:
    data = pd.read_csv(input_file)

    end_time = data['EndTime'].max()
    print end_time
    output_file.write('end time: ' + str(end_time) + '\n')

    for x in range(0, end_time):
        if (end_time-x)%30 == 0:
            output_file.write(str(x)+'\n')
