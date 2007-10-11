#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef unsigned char UCHAR;
typedef unsigned char *LPSTR;
typedef unsigned char *LPBYTE;
typedef unsigned short u_int16_t;

// Latin-1 undefined character (code 172 (Latin-1 boolean not, "¬"))
#define NOP 172

// GSM undefined character (code 16 (GSM Delta))
#define GSM_NOP 16
// conversion tables, Latin1 to GSM and GSM to Latin1
static unsigned char gsmToLatin1Table[] = {
    //  0 '@', '£', '$', '¥', 'è', 'é', 'ù', 'ì', 
    '@', 163, '$', 165, 232, 233, 249, 236,
    //  8 'ò', 'Ç',  LF, 'Ø', 'ø',  CR, 'Å', 'å', 
    242, 199, 10, 216, 248, 13, 197, 229,
    // 16 '¬', '_', '¬', '¬', '¬', '¬', '¬', '¬',
    NOP, '_', NOP, NOP, NOP, NOP, NOP, NOP,
    // 24 '¬', '¬', '¬', '¬', 'Æ', 'æ', 'ß', 'É',
    NOP, NOP, NOP, NOP, 198, 230, 223, 201,
    // 32 ' ', '!', '"', '#', '¤', '%', '&', ''',
    ' ', '!', '"', '#', 164, '%', '&', '\'',
    // 40 '(', ')', '*', '+', ',', '-', '.', '/',
    '(', ')', '*', '+', ',', '-', '.', '/',
    // 48 '0', '1', '2', '3', '4', '5', '6', '7',
    '0', '1', '2', '3', '4', '5', '6', '7',
    // 56 '8', '9', ':', ';', '<', '=', '>', '?', 
    '8', '9', ':', ';', '<', '=', '>', '?',
    // 64 '¡', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 
    161, 'A', 'B', 'C', 'D', 'E', 'F', 'G',
    // 72 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    // 80 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
    // 88 'X', 'Y', 'Z', 'Ä', 'Ö', 'Ñ', 'Ü', '§', 
    'X', 'Y', 'Z', 196, 214, 209, 220, 167,
    // 96 '¿', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
    191, 'a', 'b', 'c', 'd', 'e', 'f', 'g',
    // 104 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 
    'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    // 112 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
    // 120 'x', 'y', 'z', 'ä', 'ö', 'ñ', 'ü', 'à', 
    'x', 'y', 'z', 228, 246, 241, 252, 224
};

static unsigned char latin1ToGsmTable[256];

void
Latin1ToGsmTableInit ()
{
    int i;
    memset ((void *) latin1ToGsmTable, GSM_NOP, 256);
    for (i = 0; i < 128; i++)
        if (gsmToLatin1Table[i] != NOP)
            latin1ToGsmTable[gsmToLatin1Table[i]] = i;
}

UCHAR hextable[16] =
    { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D',
    'E', 'F'
};

#define TOHEX(a, b)	{*b++ = hextable[a >> 4];*b++ = hextable[a&0xf];}
UCHAR bcdtable[16] =
    { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '#', 'a', 'b',
'c', '\0' };
#define BCDTOASCII(a,b) {*b++ = bcdtable[a&0xf]; *b++ = bcdtable[a >> 4];}

LPSTR
BinToHex (LPBYTE p, int len)
{
    int i;
    LPSTR str = (LPSTR) malloc (len * 3 + 1);
    LPSTR basestr = str;
    for (i = 0; i < len; i++)
    {
        TOHEX (p[i], str);
    }
    *str = '\0';
    return basestr;
}

#define HEXTOBIN(x) ( (x) >= '0' && (x) <= '9' ? ((x)-'0') : \
                    (x) >= 'A' && (x) <= 'F' ? ((x)-'A'+10) : ((x)-'a'+10))

LPBYTE
HexToBin (LPSTR p, int len)
{
    int i;
    LPBYTE out = (LPBYTE) malloc (len >> 1);
    LPBYTE out_org = out;

    for (i = 0; i < len; i += 2)
    {
        *out++ = (HEXTOBIN (p[i]) << 4) | HEXTOBIN (p[i + 1]);
    }
    return out_org;
}

LPBYTE
HexToUCS2 (LPSTR p, int len)
{
    int i;
    LPBYTE out = (LPBYTE) malloc (len >> 1);
    LPBYTE out_org = out;

    for (i = 0; i < len; i += 4)
    {
        *out++ = (HEXTOBIN (p[i + 2]) << 4) | HEXTOBIN (p[i + 3]);
        *out++ = (HEXTOBIN (p[i]) << 4) | HEXTOBIN (p[i + 1]);
    }
    return out_org;
}

inline UCHAR
Ascii2Bcd (UCHAR ch)
{
    if (ch > '0' && ch < '9')
        ch -= '0';
    else if (ch == '*')
        ch = 10;
    else if (ch == '#')
        ch = 11;
    return ch;
}

int
NumToCalledPartyBcd (char *phone, LPBYTE pRecData, int len)
{
    int i = 0;
    int j = 0;
    int num_len = strlen (phone);
    int bcd_len;
    if (phone != NULL)
    {
        if (phone[0] == '+')
        {
            bcd_len = num_len / 2;
            *pRecData++ = bcd_len + 1;
            *pRecData++ = 0x91;
            i = 1;
        }
#if 0
        else if (strContaionNotNumber (phone))
        {                       //for Alphanumeric Address type
            //The maximum length of the full address field (Address-Length, Type-of-Address and Address-Value) is 12 octets.
            int len = CStrToGsm11Enc7bit (phone, pRecData + 2, 10);
            /*The Address-Length field is an integer representation of the number of useful semi-octets within the Address-Value field,
               i.e. excludes any semi octet containing only fill bits.
               04.08 page 35
             */
            *pRecData++ = len * 2;      //
            *pRecData++ = 0xd0; //Alphanumeric
            return len + 2;
        }
#endif
        else
        {
            bcd_len = (num_len + 1) / 2;
            *pRecData++ = bcd_len + 1;
            *pRecData++ = 0x81;
            i = 0;
        }
        j = 2;
        int max_len;
        if (len == -1)
        {
            len = bcd_len + 2;
            max_len = len;
        }
        else
            max_len = len - 2;
        for (; i < num_len && j < max_len; i += 2)
        {
            UCHAR chL = Ascii2Bcd (phone[i]);
            UCHAR chH;
            if (i + 1 < num_len)
                chH = Ascii2Bcd (phone[i + 1]);
            else
                chH = 0xf;
            *pRecData++ = ((chH & 0xf) << 4) | (chL & 0xf);
            j++;
            if (i + 3 == num_len)
            {
                UCHAR chL = Ascii2Bcd (phone[i + 2]);
                UCHAR chH = 0xf;
                *pRecData++ = ((chH & 0xf) << 4) | (chL & 0xf);
                j++;
                i++;
            }
        }
    }
    for (; j < len; j++)
        *pRecData++ = 0xff;
    return len;
}

u_int16_t *utf8_to_ucs2 (const unsigned char *str, int len, int *ucs2_len);
int
utf8_ucs2 (char *utf8_str, LPBYTE pData, int len)
{
    int ucs2_len = 0;
    u_int16_t *ucs2_str = NULL;
    int i;
    int utf8_len = strlen (utf8_str);
    //for(i=0; i<utf8_len; i++)
    //  fprintf(stderr,"%02X ",(unsigned char)utf8_str[i]);
    //fprintf(stderr,"\n");
    if (utf8_len > len)
        utf8_len = len;
    ucs2_str =
        utf8_to_ucs2 ((const unsigned char *) utf8_str, utf8_len, &ucs2_len);
    for (i = 0; i < ucs2_len * 2 && i < len; i += 2)
    {
        //fprintf(stderr,"%x\n",*ucs2_str);
        pData[i] = *ucs2_str >> 8;
        pData[i + 1] = *ucs2_str & 0xff;
        ucs2_str++;
    }
    return i;
}

LPSTR
PhoneNumToAscii (LPBYTE p, int len)
{
    int i;
    LPSTR str = (LPSTR) malloc (len * 2 + 10);
    LPSTR basestr = str;
    if (*p++ & 0x50)
        *str++ = '+';
    for (i = 0; i < len; i++)
    {
        BCDTOASCII (p[i], str);
        if (*(str - 1) == '\0')
            break;
    }
    *str = '\0';
    return basestr;
}

int
StrToGsm11Enc7bit (LPSTR str, LPBYTE pRecData, int len)
{
    int str_len = strlen (str);
    unsigned char *src = str;
    unsigned char *dst = pRecData;

    unsigned char next7bit;
    unsigned char shift_buf;
    char shift_n;
    int i;
    unsigned char tmp;

    shift_buf = *src++;
    i = 1;
    do
    {
        if ((i > str_len) || ((dst - pRecData) >= len))
            break;
        shift_n = (i & 7);
        next7bit = *src++;
        tmp = next7bit << (8 - shift_n);
        shift_buf |= tmp;
        *dst++ = shift_buf;
        i++;
        if (shift_n == 7)
        {
            shift_buf = *src++;
            i++;
        }
        else
            shift_buf = (next7bit >> shift_n);
    }
    while (1);
    if (str_len % 8 == 7)
        *(dst - 1) |= (13 << 1);
    return (dst - pRecData);
}


extern unsigned char *ucs2_to_utf8 (u_int16_t * ucs2_str, int len);

LPSTR
Gsm11Enc7bitToUTF8 (LPBYTE pData, int len)
{
    LPSTR str = (LPSTR) malloc (len * 8 / 7 + 1);
    LPSTR base_str = str;
    unsigned int shift_buf;
    int remanent_bits, consume_len;

    shift_buf = *pData++;
    remanent_bits = 8;
    consume_len = 1;

    do
    {
        *str++ = (unsigned char) (shift_buf & 0x7f);
        shift_buf >>= 7;
        remanent_bits -= 7;
        if (remanent_bits < 7)
        {
            if (consume_len >= len)
                break;
            shift_buf |= (*pData++ << remanent_bits);
            remanent_bits += 8;
            consume_len++;
        }
    }
    while (1);
    *str = 0;

    int str_len = str - base_str;
    unsigned short *wstr =
        (unsigned short *) malloc (sizeof (unsigned short) * str_len + 1);
    int i;
    for (i = 0; i < str_len; i++)
    {
        wstr[i] = gsmToLatin1Table[base_str[i]]<<8;
    }
    wstr[i] = 0;
    unsigned char *utf8_str = ucs2_to_utf8 (wstr, str_len);
    free (base_str);
    free (wstr);
    return utf8_str;
}

LPSTR
Gsm11SCTStoStr (LPBYTE p, int len)
{
    //e.g 50 70 90 71 81 12 23 to 05-07-09 17:18:12
    LPSTR str = (LPSTR) malloc (32);
    LPSTR basestr = str;

    BCDTOASCII (*p, str);
    p++;
    *str++ = '-';
    BCDTOASCII (*p, str);
    p++;
    *str++ = '-';
    BCDTOASCII (*p, str);
    p++;
    *str++ = ' ';
    BCDTOASCII (*p, str);
    p++;
    *str++ = ':';
    BCDTOASCII (*p, str);
    p++;
    *str++ = ':';
    BCDTOASCII (*p, str);
    *str = '\0';
    return basestr;
}

int
DecodeSMS (LPBYTE pRecData, LPSTR *smsc, LPSTR *oa, LPSTR *text, LPSTR *date)
{

    pRecData--;
    //SMS and the PDU format http://www.dreamfabric.com/sms/
    int smsc_len = pRecData[1];
//    if (smsc_len < 12)          //have meaning data.
    {
        //process smsc
        UCHAR smsc_toa = pRecData[2];
        LPSTR smsc_num = NULL;
        if (smsc_len)
        {
            if (smsc_toa == 0xd0)       //for Alphanumeric address type
            {
                smsc_num = Gsm11Enc7bitToUTF8 (&pRecData[3], smsc_len - 1);
            }
            else
            {
                smsc_num = PhoneNumToAscii (&pRecData[2], smsc_len - 1);
            }
        }
        fprintf (stderr, "smsc: %s\n", smsc_num);
        //free (smsc_num);
	*smsc = smsc_num;

        UCHAR mr = 0;
        UCHAR vp = 7;           //
        UCHAR pdu = *(pRecData + 2 + smsc_len);
        if ((pdu & 0x03) == 1)  //SMS-SUBMIT (MS ==> SMSC)
        {
            mr = 1;
            if ((pdu & 0x18) == 0)
                vp = 0;
            else if ((pdu & 0x18) == 0x10)
                vp = 1;
        }
        LPBYTE oa_a = pRecData + 2 + smsc_len + mr + 2;
        int oa_len;
        LPSTR oa_phonenum;
        if (*oa_a == 0xd0)      //for Alphanumeric address type
        {
            oa_len = pRecData[2 + smsc_len + mr + 1] / 2;
            oa_phonenum = strdup(Gsm11Enc7bitToUTF8 (oa_a + 1, oa_len));
            oa_len += 1;
        }
        else
        {
            oa_len = (pRecData[2 + smsc_len + mr + 1] + 1) / 2 + 1;
            oa_phonenum = PhoneNumToAscii (oa_a, oa_len - 1);
        }
       fprintf (stderr, "oa_phonenum: %s\n", oa_phonenum);
	*oa = oa_phonenum;

        LPBYTE udl_a = oa_a + oa_len + 2 + vp;
        int udl = *udl_a;
        UCHAR encoding = *(oa_a + oa_len + 1);
        unsigned char *utf8_str;
        if ((encoding & 0xe8) == 0x08)
        {                       //UCS2
            utf8_str = ucs2_to_utf8 (udl_a + 1, udl / 2);
        }
        else if ((encoding & 0xec) == 0x00 || (encoding & 0xf4) == 0xf0)
        {                       //7bit default alphabet
            utf8_str = Gsm11Enc7bitToUTF8 (udl_a + 1, (udl * 7 + 7) / 8);
        }
        else if ((encoding & 0xec) == 0x40 || (encoding & 0xf4) == 0xf4)
        {                       //append a space as 8 bit ascii
            *(udl_a + udl + 1) = 0;
            utf8_str = (LPSTR) (udl_a + 1);
        }
//	int i;
//	for(i=0; i<strlen(utf8_str); i++)
 //       fprintf (stderr, "%02X ", utf8_str[i]);
	*text = utf8_str;
        if (vp == 7)
        {
            LPSTR cstr_date = Gsm11SCTStoStr (udl_a - 7, 7);
       //     fprintf (stderr, "%s\n", cstr_date);
	    *date = cstr_date;
        }
    }
}

int
ComposeSubmitSms (LPBYTE pResult, int len, char *dst, char *smsc,
                  char *utf8_str)
{
    int smsc_len = 1;
    /*
       Length of SMSC information. Here the length is 0, 
       which means that the SMSC stored in the phone should be used.
       Note: This octet is optional. On some phones this octet should be omitted! 
       (Using the SMSC stored in phone is thus implicit) 
     */
    if (smsc == NULL)
        *pResult++ = 0;
    else
    {
        smsc_len = NumToCalledPartyBcd (smsc, pResult, -1);
        pResult += smsc_len;
    }
    /*
       SMS-SUBMIT PDU
       A first octet of "11" (hex) has the following meaning: 
       Bit no   7       6      5      4      3     2      1      0 
       Name TP-RP TP-UDHI TP-SRR TP-VPF TP-VPF TP-RD TP-MTI TP-MTI 
       Value    0       0      0      1      0     0      0      1 
     */
    *pResult++ = 0x11;
    /*
       TP-Message-Reference. 
       The "00" value here lets the phone set the message reference number itself. 
     */
    *pResult++ = 0;

    int da_len = NumToCalledPartyBcd (dst, pResult, -1);
    /*
       Address-Length. Length of the sender number 
     */
    if (dst == NULL)
        *pResult = 0;
    else if (pResult[1] == 0xd0)        // for Alphanumeric address type
    {
        //do nothing
    }
    else
        *pResult = strlen (dst) - ((dst[0] == '+') ? 1 : 0);
    pResult += da_len;
    /*
       Protocol identifier 0 for text sms
     */
    *pResult++ = 0;
    /*
       Data coding scheme 0x08 for UCS2
     */
    UCHAR dc;
//      if( strContaionNotASCII(Content) )
    dc = 0x08;
//      else
//              dc = 00;
    *pResult++ = dc;
    /*
       VP Value Validity period value
       0-143
       (VP + 1) x 5 minutes (i.e 5 minutes intervals up to 12 hours)
       144-167
       12 hours + ((VP-143) x 30 minutes)
       168-196
       (VP-166) x 1 day
       197-255
       (VP - 192) x 1 week
     */
    *pResult++ = 0xff;          //max value
    int ud_offset = 4 + da_len + 1 + 1 + smsc_len;
    int ret = 0;
    if (dc == 0x08)
    {
        ret = utf8_ucs2 (utf8_str, pResult + 1, len - ud_offset);
        if (ret > 140)
            ret = 140;
        *pResult = ret;
    }
    else
    {
        fprintf (stderr, "not implement now!\n");
        assert (1);
        //int ret = CStrToGsm11Enc7bit(Content,pResult+1,len-ud_offset-1);
        //int len = Content.GetLength();
        //if(len > 160)
        //      len = 160;
        //*pResult = len;
    }
    return ud_offset + ret;
}

#if 0
int
main (int argc, char *argv[])
{
    UCHAR buf[200];
    LPSTR strHex;
    int len;
    len = ComposeSubmitSms (buf, sizeof (buf), "13681557128", "13800138000", "hello");
    DecodeSMS(buf);
    strHex = BinToHex (buf, len);
    printf ("%d: %s\n", strlen (strHex) / 2 - 1, strHex);
    free (strHex);

}
#endif

