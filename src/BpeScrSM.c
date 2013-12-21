/*
 TO COMPILE USE THE CODE:
 
 R CMD SHLIB BpeScrSM.c BpeScrSM_Updates.c BpeScrSM_Utilities.c -lgsl -lgslcblas
 
 */

#include <stdio.h>
#include <math.h>

#include "gsl/gsl_matrix.h"
#include "gsl/gsl_linalg.h"
#include "gsl/gsl_blas.h"
#include "gsl/gsl_sort_vector.h"
#include "gsl/gsl_heapsort.h"

#include "R.h"
#include "Rmath.h"

#include "BpeScrSM.h"


/* */
void BpeScrSMmcmc(double survData[],
                    int *n,
                    int *p1,
                    int *p2,
                    int *p3,
                    double hyperParams[],
                    double startValues[],
                    double mcmcParams[],
                    int *numReps,
                    int *thin,
                    double *burninPerc,
                    int *nGam_save,          
                    double samples_beta1[],
                    double samples_beta2[],
                    double samples_beta3[],
                    double samples_mu_lam1[],
                    double samples_mu_lam2[],
                    double samples_mu_lam3[],
                    double samples_sigSq_lam1[],
                    double samples_sigSq_lam2[],
                    double samples_sigSq_lam3[],
                    double samples_J1[],
                    double samples_J2[],
                    double samples_J3[],
                    double samples_s1[],
                    double samples_s2[],
                    double samples_s3[],
                    double samples_theta[],
                    double samples_gamma[],
                    double samples_misc[],
                    double lambda1_fin[],
                    double lambda2_fin[],
                    double lambda3_fin[])
{
    GetRNGstate();
    
    int i, j, M;
    
    /* Survival Data */
    
    gsl_vector *survTime1    = gsl_vector_alloc(*n);
    gsl_vector *survTime2    = gsl_vector_alloc(*n);
    gsl_vector *survEvent1   = gsl_vector_alloc(*n);
    gsl_vector *survEvent2   = gsl_vector_alloc(*n);
    for(i = 0; i < *n; i++)
    {
        gsl_vector_set(survTime1, i, survData[(0 * *n) + i]);
        gsl_vector_set(survEvent1, i, survData[(1* *n) + i]);
        gsl_vector_set(survTime2, i, survData[(2 * *n) + i]);
        gsl_vector_set(survEvent2, i, survData[(3* *n) + i]);
    }
    
    int nP1, nP2, nP3;
    
    if(*p1 > 0) nP1 = *p1;
    if(*p1 == 0) nP1 = 1;
    if(*p2 > 0) nP2 = *p2;
    if(*p2 == 0) nP2 = 1;
    if(*p3 > 0) nP3 = *p3;
    if(*p3 == 0) nP3 = 1;
    
    gsl_matrix *survCov1     = gsl_matrix_calloc(*n, nP1);
    gsl_matrix *survCov2     = gsl_matrix_calloc(*n, nP2);
    gsl_matrix *survCov3     = gsl_matrix_calloc(*n, nP3);
    
    if(*p1 >0)
    {
        for(i = 0; i < *n; i++)
        {
            for(j = 0; j < *(p1); j++)
            {
                gsl_matrix_set(survCov1, i, j, survData[((4+j)* *n) + i]);
            }
        }
    }
    
    if(*p2 >0)
    {
        for(i = 0; i < *n; i++)
        {
            for(j = 0; j < *(p2); j++)
            {
                gsl_matrix_set(survCov2, i, j, survData[((4+(*p1)+j)* *n) + i]);
            }
        }
    }
    
    if(*p3 >0)
    {
        for(i = 0; i < *n; i++)
        {
            for(j = 0; j < *(p3); j++)
            {
                gsl_matrix_set(survCov3, i, j, survData[((4+(*p1)+(*p2)+j)* *n) + i]);
            }
        }
    }
    
    gsl_vector *case01   = gsl_vector_alloc(*n);
    gsl_vector *case11   = gsl_vector_alloc(*n);
    
    gsl_vector_memcpy(case01, survEvent1);
    gsl_vector_scale(case01, -1);
    gsl_vector_add_constant(case01, 1);
    gsl_vector_mul(case01, survEvent2);
    
    gsl_vector_memcpy(case11, survEvent1);
    gsl_vector_mul(case11, survEvent2);
    
    gsl_vector *yStar = gsl_vector_calloc(*n);
    gsl_vector_memcpy(yStar, survTime2);
    gsl_vector_sub(yStar, survTime1);
    
    
    /*
    for(i = 0; i < 100; i++)
    {
        printf("yStar_%d = %.3f\n", i+1, gsl_vector_get(yStar, i));
    
    }
    */
    
    /* Hyperparameters */
    
    double a1        = hyperParams[0];
    double b1        = hyperParams[1];
    double a2        = hyperParams[2];
    double b2        = hyperParams[3];
    double a3        = hyperParams[4];
    double b3        = hyperParams[5];
    double alpha1    = hyperParams[6];
    double alpha2    = hyperParams[7];
    double alpha3    = hyperParams[8];
    double c_lam1    = hyperParams[9];
    double c_lam2    = hyperParams[10];
    double c_lam3    = hyperParams[11];
    double psi       = hyperParams[12];
    double omega     = hyperParams[13];
    
    
    
    /* varialbes for birth and death moves */
    
    double C1               = mcmcParams[0];
    double C2               = mcmcParams[1];
    double C3               = mcmcParams[2];
    double delPert1         = mcmcParams[3];
    double delPert2         = mcmcParams[4];
    double delPert3         = mcmcParams[5];
    int num_s_propBI1       = mcmcParams[6];
    int num_s_propBI2       = mcmcParams[7];
    int num_s_propBI3       = mcmcParams[8];
    int J1_max              = mcmcParams[9];
    int J2_max              = mcmcParams[10];
    int J3_max              = mcmcParams[11];
    double s1_max           = mcmcParams[12];
    double s2_max           = mcmcParams[13];
    double s3_max           = mcmcParams[14];
    
    gsl_vector *s_propBI1    = gsl_vector_calloc(num_s_propBI1);
    gsl_vector *s_propBI2    = gsl_vector_calloc(num_s_propBI2);
    gsl_vector *s_propBI3    = gsl_vector_calloc(num_s_propBI3);
    for(j = 0; j < num_s_propBI1; j++) gsl_vector_set(s_propBI1, j, mcmcParams[18+j]);
    for(j = 0; j < num_s_propBI2; j++) gsl_vector_set(s_propBI2, j, mcmcParams[18+num_s_propBI1+j]);
    for(j = 0; j < num_s_propBI3; j++) gsl_vector_set(s_propBI3, j, mcmcParams[18+num_s_propBI1+num_s_propBI2+j]);
   
    
    /* time points where lambda values are stored  */

    int nTime_lambda1 = mcmcParams[15];
    int nTime_lambda2 = mcmcParams[16];
    int nTime_lambda3 = mcmcParams[17];
    
    gsl_vector *time_lambda1 = gsl_vector_calloc(nTime_lambda1);
    gsl_vector *time_lambda2 = gsl_vector_calloc(nTime_lambda2);
    gsl_vector *time_lambda3 = gsl_vector_calloc(nTime_lambda3);
    
    for(i = 0; i < nTime_lambda1; i++)
    {
        gsl_vector_set(time_lambda1, i, mcmcParams[18+num_s_propBI1+num_s_propBI2+num_s_propBI3+i]);
    }
    for(i = 0; i < nTime_lambda2; i++)
    {
        gsl_vector_set(time_lambda2, i, mcmcParams[18+num_s_propBI1+num_s_propBI2+num_s_propBI3+nTime_lambda1+i]);
    }
    for(i = 0; i < nTime_lambda3; i++)
    {
        gsl_vector_set(time_lambda3, i, mcmcParams[18+num_s_propBI1+num_s_propBI2+num_s_propBI3+nTime_lambda1+nTime_lambda2+i]);
    }
    
    double mhProp_theta_var  = mcmcParams[18+num_s_propBI1+num_s_propBI2+num_s_propBI3+nTime_lambda1+nTime_lambda2+nTime_lambda3];
  
    
    
    
    /* Starting values */
    
    gsl_vector *beta1 = gsl_vector_calloc(nP1);
    gsl_vector *beta2 = gsl_vector_calloc(nP2);
    gsl_vector *beta3 = gsl_vector_calloc(nP3);
    
    if(*p1 > 0)
    {
        for(j = 0; j < *p1; j++) gsl_vector_set(beta1, j, startValues[j]);
    }
    if(*p2 > 0)
    {
        for(j = 0; j < *p2; j++) gsl_vector_set(beta2, j, startValues[j + *p1]);
    }
    if(*p3 > 0)
    {
        for(j = 0; j < *p3; j++) gsl_vector_set(beta3, j, startValues[j + *p1 + *p2]);
    }

    
    int J1              = startValues[*p1 + *p2 + *p3];
    int J2              = startValues[*p1 + *p2 + *p3 + 1];
    int J3              = startValues[*p1 + *p2 + *p3 + 2];
    double mu_lam1      = startValues[*p1 + *p2 + *p3 + 3];
    double mu_lam2      = startValues[*p1 + *p2 + *p3 + 4];
    double mu_lam3      = startValues[*p1 + *p2 + *p3 + 5];
    double sigSq_lam1   = startValues[*p1 + *p2 + *p3 + 6];
    double sigSq_lam2   = startValues[*p1 + *p2 + *p3 + 7];
    double sigSq_lam3   = startValues[*p1 + *p2 + *p3 + 8];
    double theta        = startValues[*p1 + *p2 + *p3 + 9];
    
    gsl_vector *gamma = gsl_vector_calloc(*n);
    for(i = 0; i < *n; i++)
    {
        gsl_vector_set(gamma, i, startValues[*p1 + *p2 + *p3 + 10 + i]);
    }
    
    
    gsl_vector *lambda1  = gsl_vector_calloc(J1_max+1);
    gsl_vector *lambda2  = gsl_vector_calloc(J2_max+1);
    gsl_vector *lambda3  = gsl_vector_calloc(J3_max+1);
    for(j = 0; j < (J1+1); j++) gsl_vector_set(lambda1, j, startValues[*p1 + *p2 + *p3 + 10 + *n + j]);
    for(j = 0; j < (J2+1); j++) gsl_vector_set(lambda2, j, startValues[*p1 + *p2 + *p3 + 10 + *n + (J1+1) + j]);
    for(j = 0; j < (J3+1); j++) gsl_vector_set(lambda3, j, startValues[*p1 + *p2 + *p3 + 10 + *n + (J1+1) + (J2+1) + j]);
        
    gsl_vector *s1       = gsl_vector_calloc(J1_max+1);
    gsl_vector *s2       = gsl_vector_calloc(J2_max+1);
    gsl_vector *s3       = gsl_vector_calloc(J3_max+1);
    for(j = 0; j < (J1+1); j++) gsl_vector_set(s1, j, startValues[*p1 + *p2 + *p3 + 10 + *n + (J1+1) + (J2+1) + (J3+1) + j]);
    for(j = 0; j < (J2+1); j++) gsl_vector_set(s2, j, startValues[*p1 + *p2 + *p3 + 10 + *n + (J1+1)*2 + (J2+1) + (J3+1) + j]);
    for(j = 0; j < (J3+1); j++) gsl_vector_set(s3, j, startValues[*p1 + *p2 + *p3 + 10 + *n + (J1+1)*2 + (J2+1)*2 + (J3+1) + j]);
    
    gsl_vector *xbeta1 = gsl_vector_calloc(*n);
    gsl_vector *xbeta2 = gsl_vector_calloc(*n);
    gsl_vector *xbeta3 = gsl_vector_calloc(*n);
    gsl_blas_dgemv(CblasNoTrans, 1, survCov1, beta1, 0, xbeta1);
    gsl_blas_dgemv(CblasNoTrans, 1, survCov2, beta2, 0, xbeta2);
    gsl_blas_dgemv(CblasNoTrans, 1, survCov3, beta3, 0, xbeta3);



    
    /* Calculating Sigma_lam (from W and Q) */

    
    gsl_matrix *Sigma_lam1       = gsl_matrix_calloc(J1_max+1, J1_max+1);
    gsl_matrix *Sigma_lam2       = gsl_matrix_calloc(J2_max+1, J2_max+1);
    gsl_matrix *Sigma_lam3       = gsl_matrix_calloc(J3_max+1, J3_max+1);
    gsl_matrix *invSigma_lam1    = gsl_matrix_calloc(J1_max+1, J1_max+1);
    gsl_matrix *invSigma_lam2    = gsl_matrix_calloc(J2_max+1, J2_max+1);
    gsl_matrix *invSigma_lam3    = gsl_matrix_calloc(J3_max+1, J3_max+1);
    gsl_matrix *W1               = gsl_matrix_calloc(J1_max+1, J1_max+1);
    gsl_matrix *Q1               = gsl_matrix_calloc(J1_max+1, J1_max+1);
    gsl_matrix *W2               = gsl_matrix_calloc(J2_max+1, J2_max+1);
    gsl_matrix *Q2               = gsl_matrix_calloc(J2_max+1, J2_max+1);
    gsl_matrix *W3               = gsl_matrix_calloc(J3_max+1, J3_max+1);
    gsl_matrix *Q3               = gsl_matrix_calloc(J3_max+1, J3_max+1);

    cal_Sigma(Sigma_lam1, invSigma_lam1, W1, Q1, s1, c_lam1, J1);
    cal_Sigma(Sigma_lam2, invSigma_lam2, W2, Q2, s2, c_lam2, J2);
    cal_Sigma(Sigma_lam3, invSigma_lam3, W3, Q3, s3, c_lam3, J3);

    
    /* Variables required for storage of samples */
    
    int StoreInx;
    
    gsl_vector *accept_beta1 = gsl_vector_calloc(nP1);
    gsl_vector *accept_beta2 = gsl_vector_calloc(nP2);
    gsl_vector *accept_beta3 = gsl_vector_calloc(nP3);
    
    int accept_BI1      = 0;
    int accept_DI1      = 0;
    int accept_BI2      = 0;
    int accept_DI2      = 0;
    int accept_BI3      = 0;
    int accept_DI3      = 0;
    int accept_theta    = 0;
    
    /* Compute probabilities for various types of moves */
    
    double pRP1, pRP2, pRP3, pBH1, pBH2, pBH3, pSP1, pSP2, pSP3, pBI1, pBI2, pBI3, pDI1, pDI2, pDI3, pFP, pDP, choice;
    int move, numUpdate;
    
    gsl_vector *pB1          = gsl_vector_calloc(J1_max);
    gsl_vector *pD1          = gsl_vector_calloc(J1_max);
    gsl_vector *rho_lam1_vec = gsl_vector_calloc(J1_max);
    gsl_vector *pB2          = gsl_vector_calloc(J2_max);
    gsl_vector *pD2          = gsl_vector_calloc(J2_max);
    gsl_vector *rho_lam2_vec = gsl_vector_calloc(J2_max);
    gsl_vector *pB3          = gsl_vector_calloc(J3_max);
    gsl_vector *pD3          = gsl_vector_calloc(J3_max);
    gsl_vector *rho_lam3_vec = gsl_vector_calloc(J3_max);
    double  rho_lam1, rho_lam2, rho_lam3;
    
    for(j = 0; j < J1_max; j++)
    {
        gsl_vector_set(pB1, j, c_min(1, alpha1/(j+1 + 1)));
        gsl_vector_set(pD1, j, c_min(1, (j+1)/alpha1));
    }
    for(j = 0; j < J2_max; j++)
    {
        gsl_vector_set(pB2, j, c_min(1, alpha2/(j+1 + 1)));
        gsl_vector_set(pD2, j, c_min(1, (j+1)/alpha2));
    }
    for(j = 0; j < J3_max; j++)
    {
        gsl_vector_set(pB3, j, c_min(1, alpha3/(j+1 + 1)));
        gsl_vector_set(pD3, j, c_min(1, (j+1)/alpha3));
    }
    
    for(j = 0; j < J1_max; j++) gsl_vector_set(rho_lam1_vec, j, C1/(gsl_vector_get(pB1, j) + gsl_vector_get(pD1, j)));
    for(j = 0; j < J2_max; j++) gsl_vector_set(rho_lam2_vec, j, C2/(gsl_vector_get(pB2, j) + gsl_vector_get(pD2, j)));
    for(j = 0; j < J3_max; j++) gsl_vector_set(rho_lam3_vec, j, C3/(gsl_vector_get(pB3, j) + gsl_vector_get(pD3, j)));    
    
    rho_lam1 = gsl_vector_min(rho_lam1_vec);
    rho_lam2 = gsl_vector_min(rho_lam2_vec);
    rho_lam3 = gsl_vector_min(rho_lam3_vec);
    
    numUpdate = 14;
    if(*p1 > 0) numUpdate += 1;
    if(*p2 > 0) numUpdate += 1;
    if(*p3 > 0) numUpdate += 1;
    
    
    
    
    for(M = 0; M < *numReps; M++)
    {
        
        if(J1 < J1_max)
        {
            pBI1 = rho_lam1 * c_min(1, alpha1/(J1+1));
            pDI1 = rho_lam1 * c_min(1, J1/alpha1);
        }
        if(J2 < J2_max)
        {
            pBI2 = rho_lam2 * c_min(1, alpha2/(J2+1));
            pDI2 = rho_lam2 * c_min(1, J2/alpha2);
        }
        if(J3 < J3_max)
        {
            pBI3 = rho_lam3 * c_min(1, alpha3/(J3+1));
            pDI3 = rho_lam3 * c_min(1, J3/alpha3);
        }
        if(J1 >= J1_max)
        {
            pBI1 = 0;
            pDI1 = rho_lam1 * 2;
        }
        if(J2 >= J2_max)
        {
            pBI2 = 0;
            pDI2 = rho_lam2 * 2;
        }
        if(J3 >= J3_max)
        {
            pBI3 = 0;
            pDI3 = rho_lam3 * 2;
        }
        
        /* pBI1 = 0;  pBI2 = 0;   pBI3 = 0;*/
        /*/ pDI1 = 0;  pDI2 = 0;    pDI3 = 0; */ 
        
        
        
        pDP = 0.2;
        
        double probSub = (1 - pDP - pBI1 - pBI2 - pBI3 - pDI1 - pDI2 - pDI3)/(numUpdate-7);
        
        pRP1 = (*p1 > 0) ? probSub : 0;
        pRP2 = (*p2 > 0) ? probSub : 0;
        pRP3 = (*p3 > 0) ? probSub : 0;
        pBH1 = probSub;
        pBH2 = probSub;
        pBH3 = probSub;
        pSP1 = probSub;
        pSP2 = probSub;
        pSP3 = probSub;
        pFP  = probSub;
    
        /* selecting a move */
        /* move: 1=RP1, 2=RP2, 3=RP3, 4=BH1, 5=BH2, 6=BH3, 7=SP1, 8=SP2, 9=SP3, 10=FP,
                    11=DP, 12=BI1, 13=BI2, 14=BI3, 15=DI1, 16=DI2, 17=DI3 */
        
        choice  = runif(0, 1);
        move    = 1;
        if(choice > pRP1) move = 2;
        if(choice > pRP1+pRP2) move = 3;
        if(choice > pRP1+pRP2+pRP3) move = 4;
        if(choice > pRP1+pRP2+pRP3+pBH1) move = 5;
        if(choice > pRP1+pRP2+pRP3+pBH1+pBH2) move = 6;
        if(choice > pRP1+pRP2+pRP3+pBH1+pBH2+pBH3) move = 7;
        if(choice > pRP1+pRP2+pRP3+pBH1+pBH2+pBH3+pSP1) move = 8;
        if(choice > pRP1+pRP2+pRP3+pBH1+pBH2+pBH3+pSP1+pSP2) move = 9;
        if(choice > pRP1+pRP2+pRP3+pBH1+pBH2+pBH3+pSP1+pSP2+pSP3) move = 10;
        if(choice > pRP1+pRP2+pRP3+pBH1+pBH2+pBH3+pSP1+pSP2+pSP3+pFP) move = 11;
        if(choice > pRP1+pRP2+pRP3+pBH1+pBH2+pBH3+pSP1+pSP2+pSP3+pFP+pDP) move = 12;
        if(choice > pRP1+pRP2+pRP3+pBH1+pBH2+pBH3+pSP1+pSP2+pSP3+pFP+pDP+pBI1) move = 13;
        if(choice > pRP1+pRP2+pRP3+pBH1+pBH2+pBH3+pSP1+pSP2+pSP3+pFP+pDP+pBI1+pBI2) move = 14;
        if(choice > pRP1+pRP2+pRP3+pBH1+pBH2+pBH3+pSP1+pSP2+pSP3+pFP+pDP+pBI1+pBI2+pBI3) move = 15;
        if(choice > pRP1+pRP2+pRP3+pBH1+pBH2+pBH3+pSP1+pSP2+pSP3+pFP+pDP+pBI1+pBI2+pBI3+pDI1) move = 16;
        if(choice > pRP1+pRP2+pRP3+pBH1+pBH2+pBH3+pSP1+pSP2+pSP3+pFP+pDP+pBI1+pBI2+pBI3+pDI1+pDI2) move = 17;
        
        
        
        
        /* updating regression parameter: beta1 */
        

        
        if(move == 1)
        {
            BscrSM_updateRP1(beta1, xbeta1, accept_beta1, gamma, lambda1, s1, survTime1, survEvent1, survCov1, J1);
        }
        
        
        /* updating regression parameter: beta2 */
        

        
        if(move == 2)
        {
            BscrSM_updateRP2(beta2, xbeta2, accept_beta2, gamma, lambda2, s2, survTime1, case01, survCov2, J2);
        }
        
        
        
        
        
        /* updating regression parameter: beta3 */
        

        
        if(move == 3)
        {
            BscrSM_updateRP3(beta3, xbeta3, accept_beta3, gamma, lambda3, s3, yStar, case11, survCov3, J3);
        }
        
        

        /* updating log-baseline hazard function parameter: lambda1 */
        

      
        
        if(move == 4)
        {
            BscrSM_updateBH1(lambda1, s1, xbeta1, gamma, survTime1, survEvent1, Sigma_lam1, invSigma_lam1, W1, Q1, mu_lam1, sigSq_lam1, J1);
        }
        
        
        

        
        
        /* updating log-baseline hazard function parameter: lambda2 */
        
       

        
        if(move == 5)
        {
            BscrSM_updateBH2(lambda2, s2, xbeta2, gamma, survTime1, survTime2, case01, Sigma_lam2, invSigma_lam2, W2, Q2, mu_lam2, sigSq_lam2, J2);
        }
        
        
        
        
        /* updating log-baseline hazard function parameter: lambda3 */
        
      
   
        
        if(move == 6)
        {
            BscrSM_updateBH3(lambda3, s3, xbeta3, gamma, yStar, case11, Sigma_lam3, invSigma_lam3, W3, Q3, mu_lam3, sigSq_lam3, J3);
        }
        
        

        
        
        
        /* updating second stage survival components: mu_lam1 and sigSq_lam1 */
        

        
        if(move == 7)
        {
            BscrSM_updateSP1(&mu_lam1, &sigSq_lam1, lambda1, Sigma_lam1, invSigma_lam1, a1, b1, J1);
        }
        
        
        
        /* updating second stage survival components: mu_lam2 and sigSq_lam2 */
        

   
        
        if(move == 8)
        {
            BscrSM_updateSP2(&mu_lam2, &sigSq_lam2, lambda2, Sigma_lam2, invSigma_lam2, a2, b2, J2);
        }
        
        
        /* updating second stage survival components: mu_lam3 and sigSq_lam3 */
        
  
        
        if(move == 9)
        {
            BscrSM_updateSP3(&mu_lam3, &sigSq_lam3, lambda3, Sigma_lam3, invSigma_lam3, a3, b3, J3);
        }
        
        
        
        /* updating frailty parameter: gamma */



        
        if(move == 10)
        {
            BscrSM_updateFP(gamma, theta, xbeta1, xbeta2, xbeta3, lambda1, lambda2, lambda3, s1, s2, s3, J1, J2, J3, survTime1, yStar, survEvent1, survEvent2);
        }
        
        
        /* updating variance parameter: theta */
       

        
        
        
        if(move == 11)
        {
            BscrSM_updateDP(gamma, &theta, mhProp_theta_var, psi, omega, &accept_theta);
        }
        
        
        
        
        
        /* Updating the number of splits and their positions: J1 and s1 (Birth move) */
        

        
        if(move == 12)
        {
            BscrSM_updateBI1(s1, &J1, &accept_BI1, survTime1, survEvent1, gamma, xbeta1, Sigma_lam1, invSigma_lam1, W1, Q1, lambda1, s_propBI1, num_s_propBI1, delPert1, alpha1, c_lam1, mu_lam1, sigSq_lam1, s1_max);
        }
        
        
        /* Updating the number of splits and their positions: J2 and s2 (Birth move) */
        
        
        
        if(move == 13)
        {
            BscrSM_updateBI2(s2, &J2, &accept_BI2, survTime1, survTime2, case01, gamma, xbeta2, Sigma_lam2, invSigma_lam2, W2, Q2, lambda2, s_propBI2, num_s_propBI2, delPert2, alpha2, c_lam2, mu_lam2, sigSq_lam2, s2_max);
        }
        
        
        
        
        /* Updating the number of splits and their positions: J3 and s3 (Birth move) */
        
        
        
        if(move == 14)
        {
            BscrSM_updateBI3(s3, &J3, &accept_BI3, survTime1, yStar, case11, gamma, xbeta3, Sigma_lam3, invSigma_lam3, W3, Q3, lambda3, s_propBI3, num_s_propBI3, delPert3, alpha3, c_lam3, mu_lam3, sigSq_lam3, s3_max);
        }
        
        
        /* Updating the number of splits and their positions: J1 and s1 (Death move) */
        

        
        if(move == 15)
        {
            BscrSM_updateDI1(s1, &J1, &accept_DI1, survTime1, survEvent1, gamma, xbeta1, Sigma_lam1, invSigma_lam1, W1, Q1, lambda1, s_propBI1, num_s_propBI1, delPert1, alpha1, c_lam1, mu_lam1, sigSq_lam1, s1_max, J1_max);
        }

        /* Updating the number of splits and their positions: J2 and s2 (Death move) */
        
        
        
        if(move == 16)
        {
            BscrSM_updateDI2(s2, &J2, &accept_DI2, survTime1, survTime2, case01, gamma, xbeta2, Sigma_lam2, invSigma_lam2, W2, Q2, lambda2, s_propBI2, num_s_propBI2, delPert2, alpha2, c_lam2, mu_lam2, sigSq_lam2, s2_max, J2_max);
        }
        
        
        
        
        /* Updating the number of splits and their positions: J1 and s1 (Death move) */
        
        
        
        if(move == 17)
        {
            BscrSM_updateDI3(s3, &J3, &accept_DI3, survTime1, yStar, case11, gamma, xbeta3, Sigma_lam3, invSigma_lam3, W3, Q3, lambda3, s_propBI3, num_s_propBI3, delPert3, alpha3, c_lam3, mu_lam3, sigSq_lam3, s3_max, J3_max);
        }
        
        
        
        
        
        
        /*
        
        rep = 1000
        
        thin = 10
        
        per = 0.25
        
        
        
        
        1000 / 10 * (1-0.25) = 75
         
         
        M + 1 = 260
         
         260 > 1000 * 0.25 = 250
         
         storeInx = 260 / 10 - (1000 * 0.25)/10 = 26 - 25
        
        */
        
        
        /* Storing posterior samples */
        

        if( ( (M+1) % *thin ) == 0 && (M+1) > (*numReps * *burninPerc))
        {
            StoreInx = (M+1)/(*thin) - (*numReps * *burninPerc)/(*thin);
            
            if(*p1 >0)
            {
                for(j = 0; j < *p1; j++) samples_beta1[(StoreInx - 1) * (*p1) + j] = gsl_vector_get(beta1, j);
            }
            if(*p2 >0)
            {
                for(j = 0; j < *p2; j++) samples_beta2[(StoreInx - 1) * (*p2) + j] = gsl_vector_get(beta2, j);
            }
            if(*p3 >0)
            {
                for(j = 0; j < *p3; j++) samples_beta3[(StoreInx - 1) * (*p3) + j] = gsl_vector_get(beta3, j);
            }
            
            samples_theta[StoreInx - 1] = theta;            
            
            for(i = 0; i < nTime_lambda1; i++)
            {
                j = 0;
                while(gsl_vector_get(time_lambda1, i) > gsl_vector_get(s1, j))
                {
                    j += 1;
                }
                lambda1_fin[(StoreInx - 1) * (nTime_lambda1) + i] = gsl_vector_get(lambda1, j);
            }
            

            for(i = 0; i < nTime_lambda2; i++)
            {
                j = 0;
                while(gsl_vector_get(time_lambda2, i) > gsl_vector_get(s2, j))
                {
                    j += 1;
                }
                lambda2_fin[(StoreInx - 1) * (nTime_lambda2) + i] = gsl_vector_get(lambda2, j);
            }

            
            
            for(i = 0; i < nTime_lambda3; i++)
            {
                j = 0;
                while(gsl_vector_get(time_lambda3, i) > gsl_vector_get(s3, j))
                {
                    j += 1;
                }
                lambda3_fin[(StoreInx - 1) * (nTime_lambda3) + i] = gsl_vector_get(lambda3, j);
            }
             
                                    /* */

            
            samples_mu_lam1[StoreInx - 1] = mu_lam1;
            samples_mu_lam2[StoreInx - 1] = mu_lam2;
            samples_mu_lam3[StoreInx - 1] = mu_lam3;
            samples_sigSq_lam1[StoreInx - 1] = sigSq_lam1;
            samples_sigSq_lam2[StoreInx - 1] = sigSq_lam2;
            samples_sigSq_lam3[StoreInx - 1] = sigSq_lam3;   
            samples_J1[StoreInx - 1] = J1;
            samples_J2[StoreInx - 1] = J2;
            samples_J3[StoreInx - 1] = J3;
            
            for(j = 0; j < J1+1; j++)
            {
                samples_s1[(StoreInx - 1) * (J1_max+1) + j] = gsl_vector_get(s1, j);
            }
            for(j = 0; j < J2+1; j++)
            {
                samples_s2[(StoreInx - 1) * (J2_max+1) + j] = gsl_vector_get(s2, j);
            }
            for(j = 0; j < J3+1; j++)
            {
                samples_s3[(StoreInx - 1) * (J3_max+1) + j] = gsl_vector_get(s3, j);
            }
            
            if(*nGam_save == *n)
            {
                for(i = 0; i < *n; i++)
                {
                    samples_gamma[(StoreInx - 1) * (*n) + i] = gsl_vector_get(gamma, i);
                }
            }
            if(*nGam_save < *n)
            {
                for(i = 0; i < *nGam_save; i++)
                {
                    samples_gamma[(StoreInx - 1) * (*nGam_save) + i] = gsl_vector_get(gamma, i);
                }
            }
            
  
        }

        
        if(M == (*numReps - 1))
        {
            if(*p1 >0)
            {
                for(j = 0; j < *p1; j++) samples_misc[j] = (int) gsl_vector_get(accept_beta1, j);
            }
            if(*p2 >0)
            {
                for(j = 0; j < *p2; j++) samples_misc[*p1 + j] = (int) gsl_vector_get(accept_beta2, j);
            }
            if(*p3 >0)
            {
                for(j = 0; j < *p3; j++) samples_misc[*p1 + *p2 + j] = (int) gsl_vector_get(accept_beta3, j);
            }

            samples_misc[*p1 + *p2 + *p3] = accept_BI1;
            samples_misc[*p1 + *p2 + *p3 + 1] = accept_DI1;
            samples_misc[*p1 + *p2 + *p3 + 2] = accept_BI2;
            samples_misc[*p1 + *p2 + *p3 + 3] = accept_DI2;
            samples_misc[*p1 + *p2 + *p3 + 4] = accept_BI3;
            samples_misc[*p1 + *p2 + *p3 + 5] = accept_DI3;
            samples_misc[*p1 + *p2 + *p3 + 6] = accept_theta;
        }
        
        
        /*
        printf("move = %d\n", move);
        */

   
    }    
   
    
  
    

    
    
    
    /*
     for(i = 0; i < 10; i++)
     {
     printf("xbeta1%d = %.3f\n", i+1, gsl_vector_get(xbeta1, i));
     }
     */
    
    
    
  
    /*
  
    
    for(i = 0; i < 10; i++)
    {
        printf("case01 %d = %.f\n", i+1, gsl_vector_get(case01, i));
        printf("case11 %d = %.f\n\n", i+1, gsl_vector_get(case11, i));
    }
    
    
    */

    
  
    
        /*
    
     for(i = 0; i < J+1; i++){
     for(j = 0; j < J+1; j++)
     {
     printf("Q%d,%d =, %.6f\n", i+1, j+1, gsl_matrix_get(Q, i, j));
     }
     
     }
     
     
     for(i = 0; i < J+1; i++){
     for(j = 0; j < J+1; j++)
     {
     printf("W%d,%d =, %.6f\n", i+1, j+1, gsl_matrix_get(W, i, j));
     }
     
     }
     
     
     for(i = 0; i < J+1; i++){
     for(j = 0; j < J+1; j++)
     {
     printf("Sigma_lam%d,%d =, %.6f\n", i+1, j+1, gsl_matrix_get(Sigma_lam, i, j));
     }
     
     }
     
     for(i = 0; i < J+1; i++){
     for(j = 0; j < J+1; j++)
     {
     printf("invSigma_lam%d,%d =, %.20f\n", i+1, j+1, gsl_matrix_get(invSigma_lam, i, j));
     }
     
     }
          */
     

        /*
    printf("a1 = %.3f\n", a1);
    printf("a2 = %.3f\n", a2);
    printf("a3 = %.3f\n", a3);
    printf("b1 = %.3f\n", b1);
    printf("b2 = %.3f\n", b2);
    printf("b3 = %.3f\n", b3);
    
    printf("alpha1 = %.3f\n", alpha1);
    printf("alpha2 = %.3f\n", alpha2);
    printf("alpha3 = %.3f\n", alpha3);
    printf("c_lam1 = %.3f\n\n", c_lam1);
    printf("c_lam2 = %.3f\n\n", c_lam2);
    printf("c_lam3 = %.3f\n\n", c_lam3);
    
    for(j = 0; j < *p1; j++) printf("beta1%d = %.3f\n", j+1, gsl_vector_get(beta1, j));
    for(j = 0; j < *p2; j++) printf("beta2%d = %.3f\n", j+1, gsl_vector_get(beta2, j));
    for(j = 0; j < *p3; j++) printf("beta3%d = %.3f\n", j+1, gsl_vector_get(beta3, j));

    printf("J1 = %d\n", J1);
    printf("J2 = %d\n", J2);
    printf("J3 = %d\n", J3);
    printf("mu_lam1 = %.3f\n", mu_lam1);
    printf("mu_lam2 = %.3f\n", mu_lam2);
    printf("mu_lam3 = %.3f\n", mu_lam3);
    printf("sigSq_lam1 = %.3f\n\n", sigSq_lam1);
    printf("sigSq_lam2 = %.3f\n\n", sigSq_lam2);
    printf("sigSq_lam3 = %.3f\n\n", sigSq_lam3);
    
    for(j = 0; j < (J1+1); j++) printf("lambda1_%d = %.3f\n", j+1, gsl_vector_get(lambda1, j));
    for(j = 0; j < (J2+1); j++) printf("lambda2_%d = %.3f\n", j+1, gsl_vector_get(lambda2, j));
    for(j = 0; j < (J3+1); j++) printf("lambda3_%d = %.3f\n", j+1, gsl_vector_get(lambda3, j));

    for(j = 0; j < (J1+1); j++) printf("s1_%d = %.3f\n", j+1, gsl_vector_get(s1, j));
    for(j = 0; j < (J2+1); j++) printf("s2_%d = %.3f\n", j+1, gsl_vector_get(s2, j));
    for(j = 0; j < (J3+1); j++) printf("s3_%d = %.3f\n", j+1, gsl_vector_get(s3, j));

    printf("C1 = %.3f\n", C1);
    printf("C2 = %.3f\n", C2);
    printf("C3 = %.3f\n", C3);
    printf("delPert1 = %.3f\n", delPert1);
    printf("delPert2 = %.3f\n", delPert2);
    printf("delPert3 = %.3f\n", delPert3);
 
    printf("J1_max = %d\n", J1_max);
    printf("s1_max = %.3f\n", s1_max);
    printf("J2_max = %d\n", J2_max);
    printf("s2_max = %.3f\n", s2_max);
    printf("J3_max = %d\n", J3_max);
    printf("s3_max = %.3f\n", s3_max);
         */
    
    /*    
    for(j = 0; j < nTime_lambda1; j++) printf("time_lambda1_%d = %.3f\n", j+1, gsl_vector_get(time_lambda1, j));


    for(j = 0; j < num_s_propBI1; j++) printf("s_propBI1_%d = %.3f\n", j+1, gsl_vector_get(s_propBI1, j));
    for(j = 0; j < num_s_propBI2; j++) printf("s_propBI2_%d = %.3f\n", j+1, gsl_vector_get(s_propBI2, j));
    for(j = 0; j < num_s_propBI3; j++) printf("s_propBI3_%d = %.3f\n", j+1, gsl_vector_get(s_propBI3, j));
    */



    PutRNGstate();
    return;
    
    
}






