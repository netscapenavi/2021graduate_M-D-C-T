import csv
import glob, os
import tensorflow, numpy

block_size=1024 #여기에는 MDCT를 수행한 block 하나당 PCM sample 수가 ""아니라"", csv 파일에 저장된 block 하나 당 MDCT coeffcient 수를 적는다.
sampling_rate=44100

def erb_weighted_mse(realvalue,prediction):
    unit_freqhz=sampling_rate/(block_size*2)

coef_max_value=block_size*1.404296875
learning_rate_calc=0.01
node_in_coeff=[[]]
inblock=[]
outblock=[]
ieval=[]
oeval=[]

files_train=os.getcwd()
files_train+="\\training" #이 스크립트가 있는 경로의 'training'이라는 이름이 있는 폴더 내의 csv 파일을 읽음.
csv_files_train=glob.glob(os.path.join(files_train, "*.csv"))
csv_files_train.sort() #파일 이름 기준으로 알파벳 순으로 정렬. 이걸 이용해 '(파일 이름)_i.csv'는 neural network의 input node,
                        # '(파일 이름)_o.csv'는 neural network의 output node에 입력하는 것과 같은 방식으로 지정할 수 있다.

files_eval=os.getcwd()
files_eval+="\\evaluate" #이 스크립트가 있는 경로의 'evaluate'이라는 이름이 있는 폴더 내의 csv 파일을 읽음.
csv_files_eval=glob.glob(os.path.join(files_eval, "*.csv"))
csv_files_eval.sort()

nn_model=tensorflow.keras.models.Sequential()
nn_model.add(tensorflow.keras.layers.InputLayer(input_shape=block_size*2))
nn_model.add(tensorflow.keras.layers.Dense(units=block_size,activation='tanh'))
nn_model.add(tensorflow.keras.layers.Dense(units=block_size,activation='tanh'))
nn_model.add(tensorflow.keras.layers.Dense(units=block_size*2,activation='linear'))
i=0
while (i<len(csv_files_train)):
    with open(csv_files_train[i],'r') as nn_csv: #As long as the commands are within the 'with' namespace, the csv file will remain open.
        str_coeff=csv.reader(nn_csv, delimiter=',', quoting=csv.QUOTE_NONNUMERIC) #Reading the csv file.
        tempblock=list(str_coeff) #서로 다른 수의 MDCT coefficient block을 저장하고 있는 csv 파일을
                                 # 순수한 float32로 이루어진 list로만 구성된 하나의 numpy array에 넣기 위한 방법.
    j=0
    while j<len(tempblock):
        if (j%2==0):
            saveblock=[]
            saveblock=tempblock[j]
        else:
            saveblock=saveblock+tempblock[j]
            inblock.append(saveblock)
        j=j+1
    with open(csv_files_train[i+1],'r') as nn_csv:
        str_coeff=csv.reader(nn_csv, delimiter=',', quoting=csv.QUOTE_NONNUMERIC)
        tempblock=list(str_coeff)
    j=0
    while j<len(tempblock):
        if (j%2==0):
            saveblock=[]
            saveblock=tempblock[j]
        else:
            saveblock=saveblock+tempblock[j]
            outblock.append(saveblock)
        j=j+1
    i=i+2
i=0
while i<len(csv_files_eval):
    with open(csv_files_eval[i],'r') as nn_csv:  # As long as the commands are within the 'with' namespace, the csv file will remain open.
        str_coeff = csv.reader(nn_csv, delimiter=',', quoting=csv.QUOTE_NONNUMERIC)  # Reading the csv file.
        tempblock = list(str_coeff)
    j=0
    while j<len(tempblock):
        if (j%2==0):
            saveblock=[]
            saveblock=tempblock[j]
        else:
            saveblock=saveblock+tempblock[j]
            ieval.append(saveblock)
        j=j+1
    with open(csv_files_eval[i+1], 'r') as nn_csv:
        str_coeff = csv.reader(nn_csv, delimiter=',', quoting=csv.QUOTE_NONNUMERIC)
        tempblock = list(str_coeff)
    j=0
    while j<len(tempblock):
        if (j%2==0):
            saveblock=[]
            saveblock=tempblock[j]
        else:
            saveblock=saveblock+tempblock[j]
            oeval.append(saveblock)
        j=j+1
    i=i+2
i=0
icoef=1/coef_max_value
itemp=numpy.array(inblock)*icoef

otemp=numpy.array(outblock)*icoef

ievaltemp=numpy.array(ieval)*icoef

oevaltemp=numpy.array(oeval)*icoef

nn_model.compile(optimizer=tensorflow.keras.optimizers.SGD(learning_rate=learning_rate_calc,momentum=0.1),
          loss=tensorflow.keras.losses.mean_squared_error)

for i in range(0,50):
    nn_model.fit(itemp,otemp,epochs=5)
    tempoutcome=nn_model.evaluate(ievaltemp,oevaltemp)

#neural network에서 얻은 MDCT coefficient를 IMDCT 프로그램에서 받아들이는 csv 파일 형식으로 작성해야 한다.
with open('new_nn_coefficient.csv','w+',newline='') as new_coe: #Using option 'w+' creates a new file or clears the existing content of the already existing file.
    new_nn_nodes=csv.writer(new_coe,delimiter=',')             #"newline=''" prevents writing excessive newlines to a csv file.
    for row in node_in_coeff:
        new_nn_nodes.writerow(row)
