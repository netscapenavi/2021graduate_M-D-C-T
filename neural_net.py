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

def spreading_function(bark1, bark2): #From ISO 13818-7
    if bark2>=bark1:
        tempx=3.0*(bark2-bark1)
    else:
        tempx=1.5*(bark2-bark1)
    temp=(tempx-0.5)**2-2*(tempx-0.5)
    if temp<0:
        tempz=8*temp
    else:
        tempz=0
    tempy=15.811389+7.5*(tempx+0.474)-17.5*math.sqrt(1.0+(tempx+0.474)**2)
    if tempy<-100:
        return 0
    else:
        return pow(10,(tempy+tempz)*0.1)

def freq_to_bark_traunmuller(freqhz):
    bark=26.81*freqhz/(1960+freqhz)-0.53
    if bark<2:
        bark=0.3+0.85*bark
    elif bark>20.1:
        bark=1.22*bark-4.422
    return bark

def freq_to_bark_traunmuller_tensor(freqhz_list):
    freqhz_list_tensor = tensorflow.constant(freqhz_list)
    bark_list = 26.81 * freqhz_list_tensor / (1960 + freqhz_list_tensor) - 0.53
    cond_lowfreq = tensorflow.where(bark_list < 2.0, 0.3 + 0.85 * bark_list, tensorflow.zeros_like(bark_list))
    cond_highfreq = tensorflow.where(bark_list > 20.1, 1.22 * bark_list - 4.422, tensorflow.zeros_like(bark_list))
    cond_middlefreq = tensorflow.where(tensorflow.logical_and(bark_list >= 2.0, bark_list <= 20.1), bark_list,
                                       tensorflow.zeros_like(bark_list))
    return cond_lowfreq + cond_highfreq + cond_middlefreq

max_half_bark_scale=math.ceil(freq_to_bark_traunmuller(sampling_rate*0.5)*2)
freqhz_list=[]
window_bark_scale=[]
crit_band_edge=[]
spreading_func_table=[]
half_bark_idx_edge=[0]
prev_half_bark=0
for i in range(0,block_size):
    this_freq=(i+0.5)*unit_freqhz
    window_bark_scale.append(freq_to_bark_traunmuller(this_freq))
    if math.floor(window_bark_scale[i]*2)>prev_half_bark:
        half_bark_idx_edge.append(i)
        prev_half_bark=math.floor(window_bark_scale[i]*2)

if half_bark_idx_edge[len(half_bark_idx_edge)-1]<max_half_bark_scale:
    half_bark_idx_edge.append(max_half_bark_scale)
half_bark_idx_edge_tensor=tensorflow.constant(half_bark_idx_edge)

for i in range(0,max_half_bark_scale):
    tempbark=[]
    for j in range(0,max_half_bark_scale):
        tempbark.append(spreading_function(i*0.5,j*0.5))
    spreading_func_table.append(tempbark)
spreading_func_table_tensor=tensorflow.constant(spreading_func_table)

@tensorflow.function
def crit_band_power_calc_segment(freq_coef_power):
    isright=tensorflow.Variable(0)
    crit_band_power=None
    while tensorflow.less(isright,2):
        i = tensorflow.Variable(0)
        while tensorflow.less(i,block_size):
            crit_band_power[:,i+isright*max_half_bark_scale:i+isright*max_half_bark_scale+tensorflow.constant(1)]+=freq_coef_power[half_bark_idx_edge_tensor[i+isright*tensorflow.constant(block_size):i+tensorflow.constant(1)+isright*tensorflow.constant(block_size)]]
            i+=1
        isright+=1


def tonality_calc(freq_coef_power):
    i=tensorflow.Variable(0)

    arithmetic_mean = tensorflow.reduce_mean(freq_coef_power[half_bark_idx_edge_tensor[i:i+tensorflow.constant(1)]], axis=-1)
    geometric_mean = tensorflow.reduce_mean(tensorflow.math.log(freq_coef_power[half_bark_idx_edge_tensor[i:i+tensorflow.constant(1)]], axis=-1))
    tonality=tensorflow.concat(10*(geometric_mean-tensorflow.math.log(arithmetic_mean))/tensorflow.math.log(10), axis=0)
    return tonality

def calc_masking_threshold(freq_coef_power):
    crit_band_power=None
    spectral_flatness=tonality_calc(freq_coef_power)

def pam_weighted_mse(y_true,y_pred): #inputs are tensorflow tensors, output should be tensorflow scalar.
    #coef_max_value는 normalize된 coefficient를 다시 복구하는 것.
    canhear=2.0
    canthear=0.5
    # y_true_power = tensorflow.constant(coef_max_value)*y_true*y_true
    # y_pred_power = tensorflow.constant(coef_max_value)*y_pred*y_pred
    # masking_threshold_true=calc_masking_threshold(y_true_power)
    # compare_true=masking_threshold_true-y_true_power
    # masking_threshold_pred=calc_masking_threshold(y_true_power)
    # compare_pred=masking_threshold_pred-y_pred_power
    # weight=tensorflow.where(compare_true>0 and compare_pred>0,canthear,canhear)
    weight = tensorflow.where(tensorflow.fill([1,block_size>>1],False)+tensorflow.fill([1,block_size>>1],True), canthear, canhear)
    squared_difference = tensorflow.square(y_true - y_pred)
    default_rms = tensorflow.reduce_mean(squared_difference,axis=-1)
    squared_difference *= weight
    weighted_rms=tensorflow.reduce_mean(squared_difference,axis=-1)
    weighted_rms*=(default_rms/weighted_rms);
    return weighted_rms

    # total_weighted_rms=tensorflow.constant([1])
    # return tensorflow.convert_to_tensor(total_weighted_rms)
    #squared_difference = tensorflow.square(y_true - y_pred)
    # return tensorflow.reduce_mean(squared_difference, axis=-1)

class LossHistory(tensorflow.keras.callbacks.Callback):
    def on_batch_begin(self, batch, logs={}):
        self.loss=0
    def on_batch_end(self, batch, logs={}):
        self.loss+=logs.get('loss')

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
nn_model.add(tensorflow.keras.layers.Dense(units=block_size,activation='relu',kernel_initializer=tensorflow.keras.initializers.he_normal))
nn_model.add(tensorflow.keras.layers.Dense(units=block_size,activation='relu',kernel_initializer=tensorflow.keras.initializers.he_normal))
nn_model.add(tensorflow.keras.layers.Dense(units=block_size*2,activation='linear'))
nn_model.compile(loss='mean_squared_error', optimizer=tensorflow.keras.optimizers.SGD(learning_rate=learning_rate_calc,momentum=0.9,nesterov=True), run_eagerly=True)

losssave=LossHistory()

i = 0
while i < len(csv_files_eval):
    with open(csv_files_eval[i],
              'r') as nn_csv:  # As long as the commands are within the 'with' namespace, the csv file will remain open.
        str_coeff = csv.reader(nn_csv, delimiter=',', quoting=csv.QUOTE_NONNUMERIC)  # Reading the csv file.
        tempblock = list(str_coeff)
    j = 0
    while j < len(tempblock):
        if (j-((j >>1)<<1) == 0):
            saveblock = []
            saveblock = tempblock[j]
        else:
            saveblock = saveblock + tempblock[j]
            ieval.append(saveblock)
        j = j + 1
    with open(csv_files_eval[i + 1], 'r') as nn_csv:
        str_coeff = csv.reader(nn_csv, delimiter=',', quoting=csv.QUOTE_NONNUMERIC)
        tempblock = list(str_coeff)
    j = 0
    while j < len(tempblock):
        if (j-((j >>1)<<1) == 0): #if remainder of 2 is zero.
            saveblock = []
            saveblock = tempblock[j]
        else:
            saveblock = saveblock + tempblock[j]
            oeval.append(saveblock)
        j                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              