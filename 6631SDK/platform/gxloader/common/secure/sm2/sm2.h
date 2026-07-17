#ifndef SM2_H
#define SM2_H




int sm2_sign(const unsigned int * const p,
                         const unsigned int * const a,
                         const unsigned int * const b,
                         unsigned int size_p_in_words,
                         const unsigned int * const n,
                         unsigned int size_n_in_words,
                         unsigned int h,
                         const unsigned int * const x_G,
                         const unsigned int * const y_G,
                         const unsigned int * const d,
                         unsigned int size_d_in_words,
                         const unsigned int * const k,
                         unsigned int size_k_in_words,
                         const unsigned int * const m,
                         unsigned int size_m_in_words,
                         unsigned int * const r,
                         unsigned int * const s);

int sm2_verif(const unsigned int * const p,
                          const unsigned int * const a,
                          const unsigned int * const b,
                          unsigned int size_p_in_words,
                          const unsigned int * const n,
                          unsigned int size_n_in_words,
                          unsigned int h,
                          const unsigned int * const x_G,
                          const unsigned int * const y_G,
                          const unsigned int * const x_P,
                          const unsigned int * const y_P,
                          const unsigned int * const r,
                          const unsigned int * const s,
                          const unsigned int * const m,
                          unsigned int size_m_in_words,
                          unsigned int * p_result);

#endif // SM2_H
