

extern void matrixInv(gsl_matrix *X, gsl_matrix *Xinv);

extern void c_colSums(gsl_matrix *X, gsl_vector *v);

extern void c_rowSums(gsl_matrix *X, gsl_vector *v);

extern void c_repVec_Rowmat(gsl_vector *v, gsl_matrix *X);

extern void c_repVec_Colmat(gsl_vector *v, gsl_matrix *X);

extern double c_min(double value1,
                    double value2);


extern double c_max(double value1,
                    double value2);

extern void c_solve(gsl_matrix *M,
                    gsl_matrix *Minv);


extern void set_Ind(gsl_matrix *ind_d,
                    gsl_matrix *ind_r,
                    gsl_vector *nEvent,
                    gsl_vector *s,
                    gsl_vector *survTime,
                    gsl_vector *survEvent,
                    gsl_vector *case0yleq,
                    gsl_vector *case0ygeq,
                    gsl_vector *case1yleq,
                    gsl_vector *case1ygeq,
                    double s_max,
                    int J);

extern void cal_Delta(gsl_matrix *Delta,
                      gsl_vector *survTime,
                      gsl_vector *s,
                      int J);


extern void cal_Sigma(gsl_matrix *Sigma_lam,
                      gsl_matrix *invSigma_lam,
                      gsl_matrix *W,
                      gsl_matrix *Q,
                      gsl_vector *s,
                      double c_lam,
                      int J);


extern void c_quadform_vMv(gsl_vector *v,
                           gsl_matrix *Minv,
                           double     *value);


extern void c_quadform_vMu(gsl_vector *v,
                           gsl_matrix *Minv,
                           gsl_vector *u,
                           double     *value);



extern void c_dmvnormSH(gsl_vector *x,
                        double     mu,
                        double     sigma,
                        gsl_matrix *AInv,
                        double     *value);



extern void BpeMvnCorSurv_updateRP(gsl_vector *beta,
                                   gsl_vector *xbeta,
                           gsl_vector *lambda,
                           gsl_vector *s,
                           int K,
                           gsl_vector *V,
                           gsl_vector *survTime,
                           gsl_vector *survEvent,
                           gsl_vector *cluster,
                           gsl_matrix *survCov,
                           gsl_vector *accept_beta);



extern void BpeMvnCorSurv_updateBH(gsl_vector *lambda,
                            gsl_vector *s,
                            gsl_vector *xbeta,
                            gsl_vector *V,
                            gsl_vector *survTime,
                            gsl_vector *survEvent,
                            gsl_vector *cluster,
                            gsl_matrix *Sigma_lam,
                            gsl_matrix *invSigma_lam,
                            gsl_matrix *W,
                            gsl_matrix *Q,
                            double mu_lam,
                            double sigSq_lam,
                            int K);



extern void BpeMvnCorSurv_updateSP(double *mu_lam,
                                   double *sigSq_lam,
                                   gsl_vector *lambda,
                                   gsl_matrix *Sigma_lam,
                                   gsl_matrix *invSigma_lam,
                                   double a,
                                   double b,
                                   int K);


extern void BpeMvnCorSurv_updateBI(gsl_vector *s,
                                   int *K,
                                   int *accept_BI,
                                   gsl_vector *survTime,
                                   gsl_vector *survEvent,
                                   gsl_vector *xbeta,
                                   gsl_vector *V,
                                   gsl_vector *cluster,
                                   gsl_matrix *Sigma_lam,
                                   gsl_matrix *invSigma_lam,
                                   gsl_matrix *W,
                                   gsl_matrix *Q,
                                   gsl_vector *lambda,
                                   gsl_vector *s_propBI,
                                   int num_s_propBI,
                                   double delPert,
                                   int alpha,
                                   double c_lam,
                                   double mu_lam,
                                   double sigSq_lam,
                                   double s_max);



extern void BpeMvnCorSurv_updateDI(gsl_vector *s,
                                   int *K,
                                   int *accept_DI,
                                   gsl_vector *survTime,
                                   gsl_vector *survEvent,
                                   gsl_vector *xbeta,
                                   gsl_vector *V,
                                   gsl_vector *cluster,
                                   gsl_matrix *Sigma_lam,
                                   gsl_matrix *invSigma_lam,
                                   gsl_matrix *W,
                                   gsl_matrix *Q,
                                   gsl_vector *lambda,
                                   gsl_vector *s_propBI,
                                   int num_s_propBI,
                                   double delPert,
                                   int alpha,
                                   double c_lam,
                                   double mu_lam,
                                   double sigSq_lam,
                                   double s_max,
                                   int K_max);



extern void BpeMvnCorSurv_updateCP(gsl_vector *beta,
                                   gsl_vector *V,
                                   gsl_vector *lambda,
                                   gsl_vector *s,
                                   int K,
                                   double zeta,
                                   gsl_vector *survTime,
                                   gsl_vector *survEvent,
                                   gsl_vector *cluster,
                                   gsl_matrix *survCov,
                                   gsl_vector *n_j,
                                   gsl_vector *accept_V,
                                   double mhProp_V_var);



extern void BpeMvnCorSurv_updateVP(gsl_vector *V,
                               double *zeta,
                               double rho1,
                               double rho2);





extern void BpeMvnCorSurv_logLH(gsl_vector *beta,
                                gsl_vector *xbeta,
                                gsl_vector *lambda,
                                gsl_vector *s,
                                gsl_vector *V,
                                gsl_vector *survTime,
                                gsl_vector *survEvent,
                                gsl_matrix *survCov,
                                gsl_vector *cluster,
                                int K,
                                double *val);



