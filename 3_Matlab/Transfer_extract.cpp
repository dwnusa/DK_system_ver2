#include "mex.h"
#include "okFrontPanelDLL.h"
#include <string.h>
#include <stdio.h>

bool Transfer_extract(int* dst, okCFrontPanel *dev, int numRows, int numCols)
{
    //DAQ
    unsigned char *freebuffer0 = new unsigned char[numCols * sizeof(int)];
    unsigned char *freebuffer1 = new unsigned char[numCols * sizeof(int)];
    long ret0, ret1;//, ret2, ret3;
    bool verify = true;
    
    ret0 = dev->ReadFromBlockPipeOut(0xA0, numCols, numCols * sizeof(int), freebuffer0); //featureA ������ ȹ��
    ret1 = dev->ReadFromBlockPipeOut(0xA1, numCols, numCols * sizeof(int), freebuffer1); //featureB ������ ȹ��
    if(ret0 != ret1)// && (ret1 != ret2) && (ret2 != ret3))
    {
        mexPrintf("ret are not matched\n");
        return false;
    }
    
    for (int i = 0; i < (numCols) * sizeof(int); i += sizeof(int))
    { 
        //���� ���ڵ� (ä�� : A) from freebuffer0
        //���� ���ڵ� (ä�� : B) from freebuffer1
        //���� ���ڵ� (ä�� : C) from freebuffer2
        //���� ���ڵ� (ä�� : D) from freebuffer3
        int temp_A = (short)((freebuffer0[3 + i] << 8) | (freebuffer0[2 + i]));
        int temp_B = (short)((freebuffer0[1 + i] << 8) | (freebuffer0[0 + i]));
        int temp_C = (short)((freebuffer1[3 + i] << 8) | (freebuffer1[2 + i]));
        int temp_D = (short)((freebuffer1[1 + i] << 8) | (freebuffer1[0 + i]));
        unsigned int stamp_A = (unsigned int)1;//(temp_A);// & 0x00000003);
        unsigned int stamp_B = (unsigned int)1;//(temp_B);// & 0x00000003);
        unsigned int stamp_C = (unsigned int)1;//(temp_C);// & 0x00000003);
        unsigned int stamp_D = (unsigned int)1;//(temp_D);// & 0x00000003);
        
        int x = i / 4;
        if ((stamp_A == stamp_B) & (stamp_B == stamp_C) & (stamp_C == stamp_D))
        {
            dst[x*numRows + 0] = (int)(temp_A);// >> 2);
            dst[x*numRows + 1] = (int)(temp_B);// >> 2);
            dst[x*numRows + 2] = (int)(temp_C);// >> 2);
            dst[x*numRows + 3] = (int)(temp_D);// >> 2);
        }
        else
        {
            dst[x*numRows + 0] = (int)0;
            dst[x*numRows + 1] = (int)0;
            dst[x*numRows + 2] = (int)0;
            dst[x*numRows + 3] = (int)0;
            verify = false;
        }
    }
	
    //�޸� ����
    delete [] freebuffer0;
    delete [] freebuffer1;
//     delete [] freebuffer2;
//     delete [] freebuffer3;
    
    return verify;
}

bool Reset(okCFrontPanel *dev, unsigned int* ep00wire)
{
    unsigned int temp = ep00wire[0] & 0xFFBFFFFF; //wren�� �����Ѵ�. bit22(0)
    dev->SetWireInValue( (int)0x00, (unsigned int)temp, (unsigned int)0xffffffff );
    dev->UpdateWireIns(); //wr_en ����(0)

    temp = temp | 0x40000000; //reset�� �����Ѵ�. bit30(1)
    dev->SetWireInValue((int)0x00, (unsigned int)temp, (unsigned int)0xffffffff);
    dev->UpdateWireIns();

    temp = temp & 0xBFFFFFFF; //reset�� �����Ѵ�. bit30(0)
    dev->SetWireInValue((int)0x00, (unsigned int)temp, (unsigned int)0xffffffff);
    dev->UpdateWireIns();

    temp = temp | 0x00400000; //wren�� �����Ѵ�. bit22(1)
    dev->SetWireInValue( (int)0x00, (unsigned int)temp, (unsigned int)0xffffffff );
    dev->UpdateWireIns();    
    return true;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    //Matlab Input Matrix Size
    if(nrhs != 3)
        mexErrMsgTxt("Invalid number of input arguments");
    if(nlhs != 1)
        mexErrMsgTxt("Invalid number of outputs");
    if(!mxIsInt32(prhs[0]))
        mexErrMsgTxt("input buffer type must be single");
    if(!mxIsInt32(prhs[1]))
        mexErrMsgTxt("input buffer type must be single");
    if(!mxIsUint32(prhs[2]))
        mexErrMsgTxt("input ep00wire type must be uint32");
    
    int* numRows = (int*)mxGetData(prhs[0]);
    int* numCols = (int*)mxGetData(prhs[1]);
    
    unsigned int* ep00wire = (unsigned int*)mxGetData(prhs[2]);
    
    if(numRows[0] != 4)
		mexErrMsgTxt("Invalid buffer size. It must be 4x(buffer)");
    
    
    //Opalkelly variable
	okCFrontPanel *dev = new okCFrontPanel;
    
    dev->OpenBySerial("");//14230007CO
    
    if (dev->IsOpen()) {} //mexPrintf("IsOpen Pass\n");
    else{
        mexPrintf("IsOpen Fail\n");
        dev->~okCFrontPanel();
        return;}
    if (dev->IsFrontPanelEnabled()) {}//mexPrintf("FrontPanel support is enabled.\n");
    else{
        mexPrintf("FrontPanel support is not enabled.\n");
        dev->~okCFrontPanel();
        return;}
    
    //featureABCD ����Ȯ��
    dev->UpdateWireOuts();
    int tempWireOuts = (int)(dev->GetWireOutValue(0x20));
    
    //(featureA) ����Ȯ��
    int n0 = (int)((tempWireOuts & 0x00000001) >> 0);
    //(featureB) ����Ȯ��
    int n1 = (int)((tempWireOuts & 0x00000002) >> 1);
    //(featureC) ����Ȯ��
//     int n2 = (int)((tempWireOuts & 0x00000004) >> 2);
    //(featureD) ����Ȯ��
//     int n3 = (int)((tempWireOuts & 0x00000008) >> 3);
    
    if ((n0 == 1) && (n1 == 1))
    {
        
        plhs[0] = mxCreateNumericMatrix(numRows[0], numCols[0], mxINT32_CLASS, mxREAL);
        int* out = (int*)mxGetData(plhs[0]);
        
        unsigned int temp = ep00wire[0] & 0xFFBFFFFF; //wren�� �����Ѵ�. bit22(0)
        dev->SetWireInValue( (int)0x00, (unsigned int)temp, (unsigned int)0xffffffff );
        dev->UpdateWireIns();

        bool verify = Transfer_extract(out, dev, numRows[0], numCols[0]);
        
        temp = ep00wire[0] | 0x00400000; //wren�� �����Ѵ�. bit22(1)
        dev->SetWireInValue( (int)0x00, (unsigned int)temp, (unsigned int)0xffffffff );
        dev->UpdateWireIns();
        
        if (verify == false) //����� ���� �ִ� ��� (4x1) 0 ������ ��ȯ
        {
            plhs[0] = mxCreateNumericMatrix(numRows[0], 1, mxINT32_CLASS, mxREAL);
            out = (int*)mxGetData(plhs[0]);
            Reset(dev, ep00wire);
            mexPrintf("Transfer_extract : Found stamp mismatched, Not synchronized among FIFOs\n");
        }
    }
    else
    {
        plhs[0] = mxCreateNumericMatrix(numRows[0], 1, mxINT32_CLASS, mxREAL);
        int* out = (int*)mxGetData(plhs[0]);
    }
    
    //����
    dev->~okCFrontPanel();
}
