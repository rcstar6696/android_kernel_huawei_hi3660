

#ifndef _NF_AD_FILTER
#define _NF_AD_FILTER
void uninit_ad(void);
void init_ad(void);
void add_ad_rule(const char *rule, bool reset);
void output_ad_rule(void);
void clear_ad_rule(int opt, const char *data);
bool match_ad_uid(unsigned int uid);
bool match_ad_url(unsigned int uid, const char *data, int datalen);
bool match_url_string(const char *url, const char *filter, int len);
bool is_droplist(char *url, int len);
void add_to_droplist(char *url, int len);
#endif /*_NF_AD_FILTER*/
