import csv, ctypes
import glob, os
import tensorflow, numpy, math

block_size=1024 #여기에는 MDCT를 수행한 block 하나당 PCM sample 수가 ""아니라"", csv 파일에 저장된 block 하나 당 MDCT coeffcient 수를 적는다.
inv_block_size = 1 / block_size
sampling_rate=44100
unit_freqhz=sampling_rate * 0.5 * inv_block_size

coef_max_value=block_size*1.404296875
icoef=1/coef_max_value
epoch_length=0
learning_rate_calc=0.01
eval_checknum = 4
cpu_num=0
inv_cpu_num=0
manager=None
return_dict=None

loss_per_epoch_train=[]
loss_per_epoch_eval=[]
node_in_coeff=[[]]
inblock=[]
outblock=[]
ieval=[]
oeval=[]

freqhz_list=[]
window_bark_scale=[]
crit_band_edge=[]
spreading_func_table=[]
half_bark_idx_edge=[0]
prev_half_bark=0

files_train=os.getcwd()
files_train+="\\material_to_predict" #이 스크립트가 있는 경로의 'material_to_predict'이라는 이름이 있는 폴더 내의 csv 파일을 읽음.
csv_files_train=glob.glob(os.path.join(files_train, "*.csv"))
csv_files_train.sort() #파일 이름 기준으로 알파벳 순으로 정렬. 이걸 이용해 '(파일 이름)_i.csv'는 neural network의 input node,
                        # '(파일 이름)_o.csv'는 neural network의 output node에 입력하는 것과 같은 방식으로 지정할 수 있다.

nn_model=tensorflow.keras.models.load_model('saved_model_1')

i=0
loss_per_epoch_train.append(0)
while (i<len(csv_files_train)):
    print(csv_files_train[i])
    with open(csv_files_train[i],'r') as nn_csv: #As long as the commands are within the 'with' namespace, the csv file will remain open.
        str_coeff=csv.reader(nn_csv, delimiter=',', quoting=csv.QUOTE_NONNUMERIC) #Reading the csv file.
        tempblock=list(str_coeff) #서로 다른 수의 MDCT coefficient block을 저장하고 있는 csv 파일을
                                 # 순수한 float32로 이루어진 list로만 구성된 하나의 numpy array에 넣기 위한 방법.
    j=0
    inblock=[]
    while j<len(tempblock):
        if (j-((j >>1)<<1) == 0):
            saveblock=[]
            saveblock=tempblock[j]
        else:
            saveblock=saveblock+tempblock[j]
            inblock.append(saveblock)
        j=j+1
    itemp = numpy.array(inblock) * icoef #icoef로 input을 normalize.
    otemp = nn_model.predict(itemp) * coef_max_value #coef_max_value로 output을 denormalize.
    print("Prediction of " + csv_files_train[i] + " is complete.")
    temp_csv_filename=""
    before_cut_length=len(csv_files_train[i])
    j=before_cut_length-1
    while csv_files_train[i][j-4]!='\\':
        temp_csv_filename+=csv_files_train[i][j-4]
        j=j-1
    temp_csv_filename=''.join(reversed(temp_csv_filename))
    temp_csv_filename=temp_csv_filename+"_pred.csv"
    with open(os.getcwd()+"\\predicted\\"+temp_csv_filename, 'w+', newline='') as pred_save:
        save=csv.writer(pred_save)
        indiv_stereo=[]
        for j in range(0,len(otemp)):
            indiv_stereo=otemp[j].tolist()
            save.writerow(indiv_stereo[0:block_size])
            save.writerow(indiv_stereo[block_size:2*block_size])
    pred_save.close()
    print("Prediction of " + csv_files_train[i] + " is saved.")
    i=i+1

#neural network에서 얻은 MDCT coefficient를 IMDCT 프로그램에서 받아들이는 csv 파일 형식으로 작성해야 한다.
#with open('ncoefficient.csv','w+',newline='') as new_coe: #Using option 'w+' creates a new file or clears the existing content of the already existing file.
#    new_nn_nodes=csv.writer(new_coe,delimiter=',')             #"newline=''" prevents writing excessive newlines to a csv file.
#    for row in node_in_coeff:
#        new_nn_nodes.writerow(row)
