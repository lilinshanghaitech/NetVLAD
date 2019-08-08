#define MAX_SIZE 16
typedef struct data_MAXSIZE_struct{
    float data[MAX_SIZE];
}data_MAXSIZE;
void conv_hw(const data_MAXSIZE *data,
	            const float *w,
				float *a
	            )
	{
#pragma HLS INTERFACE s_axilite port=return
#pragma HLS INTERFACE m_axi depth=18432 port=data offset=slave bundle=in_data
#pragma HLS INTERFACE m_axi depth=32768 port=w offset=slave bundle=in_w
#pragma HLS INTERFACE m_axi depth=36864 port=a offset=slave bundle=out_a
	    #pragma HLS data_pack variable=data struct_level
	    // Local memory to store input and output
    	float local_data[512][MAX_SIZE];
    	#pragma HLS ARRAY_PARTITION variable=local_data dim=2 complete

	    float local_w[MAX_SIZE][512];
	    #pragma HLS ARRAY_PARTITION variable=local_w dim=1 complete

	    float local_a[MAX_SIZE][MAX_SIZE];
	    #pragma HLS ARRAY_PARTITION variable = local_a dim = 0 complete

	    block_row:
	    for (int ii = 0; ii < 64; ii += MAX_SIZE)
	    {
	        #pragma HLS LOOP_TRIPCOUNT min=4 max=4
	        read_w:
	            for (int j = 0; j < 512; ++j)
	            {
	                for (int n = 0; n < MAX_SIZE; ++n)
	                {
						#pragma HLS PIPELINE II=1
	                	local_w[n][j] = w[ii*512 + n*512 + j];
	                }
	            }


	        block_col:
	        for (int jj = 0; jj < 576; jj += MAX_SIZE)
	        {
	            #pragma HLS LOOP_TRIPCOUNT min=36 max=36
	            read_data:
	            for (int i = 0; i < 512; ++i)
	            {
	                #pragma HLS PIPELINE II=1
	                for (int n = 0; n < MAX_SIZE; ++n)
	                {
	                    local_data[i][n] = data[i*576/MAX_SIZE + jj/MAX_SIZE].data[n];
	                }
	            }

	            systolic1:
	            for (int k = 0; k < 512; k++)
	            {
	                #pragma HLS LOOP_TRIPCOUNT min=512 max=512
	                #pragma HLS PIPELINE II=1
	                systolic2:
	                for (int i = 0; i < MAX_SIZE; i++)
	                {
	                systolic3:
	                    for (int j = 0; j < MAX_SIZE; j++)
	                    {
	                        // Get previous sum
	                        float last = (k == 0) ? 0 : local_a[i][j];
	                        // Update current sum
	                        // Handle boundary conditions
	                        float w_val = (i < 64 && k < 512) ? local_w[i][k] : 0;
	                        float data_val = (k < 512 && j < 576) ? local_data[k][j] : 0;
	                        float result = last + w_val * data_val;
	                        // Write back results
	                        local_a[i][j] = result;
	                    }
	                }
	            }

	            write_a:
	            for (int i = 0; i < MAX_SIZE; ++i)
	            {
	                for (int j = 0; j < MAX_SIZE; ++j)
	                {
						#pragma HLS PIPELINE II=1
	                    a[(ii + i)*576 + jj + j] = local_a[i][j];
	                }
	            }
	        }
	    }
	}


