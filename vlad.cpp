#define MAX_SIZE2 8
typedef struct data_MAXSIZE2_struct{
    float data[MAX_SIZE2];
}data_MAXSIZE2;
void vlad_core_hw(const data_MAXSIZE2 *data,
                const float *a,
                const float *c,
                float *v
            )
{
#pragma HLS INTERFACE s_axilite port=return
#pragma HLS INTERFACE m_axi depth=36864 port=data offset=slave bundle=in_data
#pragma HLS INTERFACE m_axi depth=36864 port=a offset=slave bundle=in_a
#pragma HLS INTERFACE m_axi depth=32768 port=c offset=slave bundle=in_c
#pragma HLS INTERFACE m_axi depth=32768 port=v offset=slave bundle=out_v

    #pragma HLS data_pack variable=data struct_level
    // Local memory to store input and output
    float local_data[512][MAX_SIZE2];
    #pragma HLS ARRAY_PARTITION variable=local_data dim=2 complete

    float local_a[MAX_SIZE2][MAX_SIZE2];
    #pragma HLS ARRAY_PARTITION variable = local_a dim = 0 complete

    float local_c[MAX_SIZE2][512];
    #pragma HLS ARRAY_PARTITION variable=local_c dim=1 complete

    float local_v[MAX_SIZE2][512];
    #pragma HLS ARRAY_PARTITION variable = local_v dim = 1 complete
    block_row:
    for (int ii = 0; ii < 64; ii += MAX_SIZE2)
    {

    	read_c:
        for (int i = 0; i < MAX_SIZE2; ++i)
        {
            for (int j = 0; j < 512; ++j)
            {
				#pragma HLS PIPELINE II=1
                local_c[i][j] = c[ii*512 + i*512 + j];
            }
        }

        block_col:
        for (int jj = 0; jj < 576; jj += MAX_SIZE2)
        {
//            #pragma HLS DATAFLOW
            read_data:
            for (int i = 0; i < 512; ++i)
            {
                #pragma HLS LOOP_TRIPCOUNT min=512 max=512
                #pragma HLS PIPELINE II=1
                for (int n = 0; n < MAX_SIZE2; ++n)
                {
                    local_data[i][n] = data[i*576/MAX_SIZE2 + jj/MAX_SIZE2].data[n];

                }
            }

            read_a:
            for (int i = 0; i < MAX_SIZE2; ++i)
            {
                for (int j = 0; j < MAX_SIZE2; ++j)
                {
                    #pragma HLS PIPELINE II=1
                    local_a[i][j] = a[ii*576 + i*576 + jj + j];
                }

            }

            core:
            for (int k = 0; k < 512; k++)
            {
                #pragma HLS LOOP_TRIPCOUNT min=512 max=512
                #pragma HLS PIPELINE II=1
                systolic2:
                for (int i = 0; i < MAX_SIZE2; i++)
                {
                systolic3:
                    for (int j = 0; j < MAX_SIZE2; j++)
                    {
                        float last_v = (jj == 0 && j == 0) ? 0 : local_v[i][k];
                        float result_v = last_v + (local_data[k][j] + local_c[i][k]) * local_a[i][j];
                        local_v[i][k] = result_v;
                    }
                }
            }
        }

        write_v:
        for (int i = 0; i < MAX_SIZE2; ++i)
        {
            for (int j = 0; j < 512; ++j)
            {
                #pragma HLS PIPELINE II=1
                v[(ii + i)*512 + j] = local_v[i][j];
            }
        }
    }
}
