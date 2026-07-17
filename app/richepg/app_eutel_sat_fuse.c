#include "app.h"

#if RICHEPG_SUPPORT
#include "app_eutel_database.h"
#include "app_eutel_common.h"
#include "richepg.h"
#include "app_eutel_sat_fuse.h"

struct _fuse_sat_item
{
	unsigned int valid   ;
	unsigned int sat_id  ;

	unsigned int freescan_id ;//free scan
	unsigned int sattv_id  ;  //sat tv

	unsigned int display_name_len ;
	unsigned int area_name_index ;

	unsigned char sat_name[MAX_SAT_NAME];
};

static struct _fuse_sat_item  s_fuse_sat[EUTUL_SAT_MAX_NUM] ;

int __app_eutel_sat_data_init(void)
{
	memset(s_fuse_sat,0,EUTUL_SAT_MAX_NUM*sizeof(struct _fuse_sat_item));

	return 0 ;
}

int app_eutel_sat_fuse_init(void)
{
	app_eutel_sat_fuse_build();
	return 0 ;
}

int app_eutel_sat_fuse_build(void)
{
    int i=0,j=0,k=0;
    int add_num = 0 ;
	unsigned int name_len , name_len2;
    GxMsgProperty_NodeByPosGet node_get = {0};
    GxBusPmViewInfo view_info = {0};
	unsigned char temp_name[MAX_SAT_NAME];

    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

    view_info.group_mode = GROUP_MODE_ALL; //all
    view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
    view_info.stream_type = GXBUS_PM_PROG_ALL;
    GxBus_PmViewInfoMemSet(&view_info); // Only memory, not flash

    // get total sat
    GxMsgProperty_NodeNumGet sat_num = {0};
    sat_num.node_type = NODE_SAT;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&sat_num));

    if ( sat_num.node_num <= 0 )
    {
        EUTEL_SINFO("sat list empty \n");
        goto end ;
    }

    add_num = 0 ;
	__app_eutel_sat_data_init();

    for(i=0; i<sat_num.node_num; i++)
    {
        memset(&node_get, 0, sizeof(GxMsgProperty_NodeByPosGet));
        node_get.node_type = NODE_SAT;
        node_get.pos = i;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);

        if ( (app_eutel_sat_type_check(node_get.sat_data.id)) && (add_num<EUTUL_SAT_MAX_NUM) )
        {
            name_len = strlen((char*)node_get.sat_data.sat_s.sat_name);

			s_fuse_sat[add_num].valid   = 1 ;
			s_fuse_sat[add_num].sat_id  = node_get.sat_data.id ;
			s_fuse_sat[add_num].sattv_id  = node_get.sat_data.id ;

			memcpy(s_fuse_sat[add_num].sat_name,node_get.sat_data.sat_s.sat_name,name_len);
			s_fuse_sat[add_num].display_name_len = name_len ;

            add_num++ ;
        }
    }

	for(i=0; i<sat_num.node_num; i++)
    {
        memset(&node_get, 0, sizeof(GxMsgProperty_NodeByPosGet));
        node_get.node_type = NODE_SAT;
        node_get.pos = i;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);

        if (!app_eutel_sat_type_check(node_get.sat_data.id))
        {
            for(j=0;j<EUTUL_SAT_MAX_NUM;j++)
			{
				if ( 0 == s_fuse_sat[j].valid )
					continue ;

				name_len  = strlen((char*)node_get.sat_data.sat_s.sat_name);
				name_len2 = strlen((char*)s_fuse_sat[j].sat_name);

				if ( name_len2 < name_len )
					continue ;

				if ( strncmp((char*)node_get.sat_data.sat_s.sat_name,(char*)s_fuse_sat[j].sat_name,name_len) == 0)
				{
					s_fuse_sat[j].freescan_id = node_get.sat_data.id ;

					if ((name_len2 -1 ) > name_len  )
					{
						s_fuse_sat[j].area_name_index = name_len + 1 ;
					}
				}
			}
        }
    }

	for(i=0;i<EUTUL_SAT_MAX_NUM;i++)
	{
		if ( !s_fuse_sat[i].valid )
			continue ;

		memset(temp_name,0,MAX_SAT_NAME);
		name_len = strlen((char*)s_fuse_sat[i].sat_name);

		for(j=0;j<name_len;j++)
		{
			if (s_fuse_sat[i].sat_name[j] == ' ')
			{
				break;
			}
			else
			{
				temp_name[j] = s_fuse_sat[i].sat_name[j] ;
			}
		}
		if ((j<=0)|| (j>= name_len))
			continue ;

		for(k=i+1;k<EUTUL_SAT_MAX_NUM;k++)
		{
			if ( strncmp((char*)temp_name,(char*)s_fuse_sat[k].sat_name,j) == 0 )
			{
				s_fuse_sat[k].sattv_id = s_fuse_sat[i].sat_id ;
				s_fuse_sat[i].sattv_id = s_fuse_sat[i].sat_id ;

				s_fuse_sat[k].display_name_len = j ;
				s_fuse_sat[i].display_name_len = j ;

				if ( s_fuse_sat[i].area_name_index == 0)
				{
					s_fuse_sat[i].area_name_index = j+1 ;
					s_fuse_sat[k].area_name_index = j+1 ;
				}
			}
		}
	}

end:
    GxBus_PmViewInfoMemClear();//recovery flash viewinfo

    return 0 ;
}

unsigned char *app_eutel_sat_get_display_name(int index,int *len)
{
	unsigned char *pname = NULL ;
	if ( (index<0) || (index >= EUTUL_SAT_MAX_NUM))
		return NULL ;
	
	if ( len )
		*len = s_fuse_sat[index].display_name_len ;
	
	pname =  s_fuse_sat[index].sat_name;
	return pname ;
}

unsigned char *app_eutel_sat_get_area_name(int index)
{
	unsigned char *pname = NULL ;
	if ( (index<0) || (index >= EUTUL_SAT_MAX_NUM))
		return NULL ;
	
	pname =  (s_fuse_sat[index].sat_name + s_fuse_sat[index].area_name_index);
	return pname ;
}

int app_eutel_sat_fuse_free_is_point(int free_sat_id)
{
    int i = 0 ;

	for(i=0;i<EUTUL_SAT_MAX_NUM;i++)
	{
		if ( (s_fuse_sat[i].valid ) && ((free_sat_id == s_fuse_sat[i].freescan_id ))
              && (s_fuse_sat[i].sat_id>0) )
		{
			return 1 ;
		}
	}
    return 0 ;
}

int app_eutel_sat_fuse_free_sat_is_point(unsigned int sat_id)
{
    int i = 0 ;

	for(i=0;i<EUTUL_SAT_MAX_NUM;i++)
	{
		if ( (s_fuse_sat[i].valid ) && (sat_id == s_fuse_sat[i].sat_id ))
		{
			if (s_fuse_sat[i].freescan_id>0)
				return 1 ;

			if ((s_fuse_sat[i].sattv_id>0) && (sat_id != s_fuse_sat[i].sattv_id ))
				return 1 ;
		}
	}
    return 0 ;
}

int app_eutel_sat_fuse_get_freescan_area_count(unsigned int free_sat_id,unsigned int *pIndex)
{
	int count = 0 ;
	int i = 0 ;

	for(i=0;i<EUTUL_SAT_MAX_NUM;i++)
	{
		if ( (s_fuse_sat[i].valid ) && ((free_sat_id == s_fuse_sat[i].freescan_id ))
              && (s_fuse_sat[i].sat_id>0) )
		{
			if (pIndex)
				pIndex[count] = i ;
			count ++ ;
		}
	}
    return count ;
}

int app_eutel_sat_fuse_get_sattv_area_count(unsigned int sattv_id,unsigned int *pIndex)
{
	int count = 0 ;
	int i = 0 ;

	for(i=0;i<EUTUL_SAT_MAX_NUM;i++)
	{
		if ( (s_fuse_sat[i].valid ) && (sattv_id == s_fuse_sat[i].sattv_id )
			  && (s_fuse_sat[i].sat_id>0))
		{
			if (pIndex)
				pIndex[count] = i ;
			count ++ ;
		}
	}
    return count ;
}

int app_eutel_sat_fuse_get_all_area_count(unsigned int sat_id,unsigned int *pIndex)
{
	if(app_eutel_sat_type_check(sat_id))
	{
		return app_eutel_sat_fuse_get_sattv_area_count(sat_id,pIndex);
	}
	else
	{
		return app_eutel_sat_fuse_get_freescan_area_count(sat_id,pIndex);
	}
}

#endif
