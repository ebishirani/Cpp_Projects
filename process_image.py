import numpy as np
from PIL import Image

# uyvyFileName = '/media/sam/DeletedC/work_dir/python-project/image_data_analys/uyvy_boomi/500.dat'
# uyvyData = np.memmap(uyvyFileName, dtype='uint8', mode='r').__array__()

# uyvyData = uyvyData.reshape(1080, 1920, -1)

# uvData = uyvyData[:, :, 0]

# yData = uyvyData[:, :, 1]

# grayImg = Image.fromarray(yData, 'P')
# grayImg.show(title='ychannel')

# grayImg2 = Image.fromarray(uvData, 'P')
# grayImg2.show(title='uvchannel')

#*********************************************************
# import numpy as np
# from PIL import Image
bgrFilename = '/media/sam/DeletedC/work_dir/python-project/image_data_analys/RGB_CPU/40.dat'
bgrData = np.memmap(bgrFilename, dtype='uint8', mode='r').__array__()
bgrData = bgrData.reshape(1080, 1920, -1)

# bgrData3Channel = bgrData[: , :, 0:3]

imgBgrx = Image.fromarray(bgrData, 'RGB')
imgBgrx.show(title='RGB')
#*********************************************************

rgbFilename = '/media/sam/DeletedC/work_dir/python-project/image_data_analys/RGBA_CPU2/40.dat'
data = np.memmap(rgbFilename, dtype='uint8', mode='r').__array__()
data = data.reshape(1080, 1920, -1)

rgbData = data[: , :, 0:3]
rgbData = rgbData - bgrData
# data = np.array(data)
# data[1:100, 1:100, 3] = 1
# data[200:300, 200:300, 3] = 220

imgRgba = Image.fromarray(data, 'RGBA')
imgRgba.show(title='RGBA')

imgRgb = Image.fromarray(rgbData, 'RGB')
imgRgb.show()


# data = (img.reshape(-1, 4)).T
# data = img.reshape(4, 1920, -1)
i=0