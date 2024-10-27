
#include "stdio.h"
#include "string.h"

#define Printf printf

static char Sb[1024];
size_t SbLen = 0;
static char Sh[1024];
size_t ShLen = 0;

static char Sbb[4096];
size_t SbbLen = 0;

void command_c1(void);
void command_c1(void)
{
    //int rtn;
    int contentLength;


    strcat( &Sb[SbLen], "\r\n"); SbLen = strlen( Sb );
    strcat( &Sb[SbLen], "----boudary_7wf\r\n"); SbLen = strlen( Sb );
    strcat( &Sb[SbLen], "Content-Disposition: form-data; name=\"tr7wf\"; filename=\"7wf-5F36FF42.dat\"\r\n"); SbLen = strlen( Sb );
    strcat( &Sb[SbLen], "Content-Type: application/octet-stream\r\n"); SbLen = strlen( Sb );
    strcat( &Sb[SbLen], "\r\n"); SbLen = strlen( Sb );
    strcat( &Sb[SbLen], "c=1,s=5F36FF42\r\n"); SbLen = strlen( Sb );
    strcat( &Sb[SbLen], "\r\n"); SbLen = strlen( Sb );
    strcat( &Sb[SbLen], "----boudary_7wf--"); SbLen = strlen( Sb );

    //contentLength = strlen(Sb);
    contentLength = SbLen;


    strcat( &Sh[ShLen], "POST /api/dev-7wf-1.php HTTP/1.0\r\n"); ShLen = strlen( Sh );
    strcat( &Sh[ShLen], "User-Agent: TandD device (TR-7WF)\r\n"); ShLen = strlen( Sh );
    strcat( &Sh[ShLen], "Host: develop.webstorage.jp\r\n"); ShLen = strlen( Sh );
    strcat( &Sh[ShLen], "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"); ShLen = strlen( Sh );
    strcat( &Sh[ShLen], "Connection: keep-alive\r\n"); ShLen = strlen( Sh );
    strcat( &Sh[ShLen], "Content-Type: multipart/form-data; boundary=--boudary_7wf\r\n"); ShLen = strlen( Sh );
    strcat( &Sh[ShLen], "Content-Length: 171\r\n"); ShLen = strlen( Sh );
//TODO   strcat( &Sh[ShLen], "Content-Length: %d\r\n", contentLength);
    strcat( &Sh[ShLen], "\r\n"); ShLen = strlen( Sh );

    /*
    Printf("---------------------------------------------\r\n");
    Printf("ShLen = %d\r\n", ShLen);
    Printf("Sh = %s\r\n", Sh);
    Printf("---------------------------------------------\r\n");
    Printf("---------------------------------------------\r\n");
    Printf("SbLen = %d\r\n", SbLen);
    Printf("Sb = %s\r\n", Sb);
    Printf("---------------------------------------------\r\n");
    */

    SbbLen = 0;
    strcat( &Sbb[SbbLen], Sh); SbbLen = strlen( Sbb );
    strcat( &Sbb[SbbLen], Sb); SbbLen = strlen( Sbb );


}
#if 0
!!!Hello World!!!
RXlen = 161


[request_headers]
POST /api/dev-7wf-1.php HTTP/1.0

User-Agent: TandD device (TR-7WF)
Host: develop.webstorage.jp
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/ *;q=0.8
Connection: keep-alive
Content-Type: multipart/form-data; boundary=--boudary_7wf
Content-Length: 171
----boudary_7wf
Content-Disposition: form-data; name="tr7wf"; filename="7wf-5F36FF42.dat"
Content-Type: application/octet-stream

c=1,s=5F36FF42
----boudary_7wf--

[response_headers]
HTTP/1.1 200 OK
Server: nginx
Date: Wed, 16 Oct 2024 07:24:29 GMT
Content-Type: text/html
Content-Length: 21
Connection: keep-alive

[body]
R=1,0



#endif


char* getBigBuffer(void);
char* getBigBuffer(void)
{
	return &Sbb[0];
}

char * get_c1(void);
char * get_c1(void)
{
	command_c1();

	char * bb;

	bb = getBigBuffer();

	return bb;
}




#if 0


        void command_c1(TRdeviceInfo _tr7Dev)
        {
            int rtn;
            int contentLength;

            string Sb = "";
            Sb += string.Format("----boudary_7wf\r\n");
            Sb += string.Format(string.Format("Content-Disposition: form-data; name=\"tr7wf\"; filename=\"7wf-{0}.dat\"\r\n", _tr7Dev.serialNoStr));
            Sb += string.Format("Content-Type: application/octet-stream\r\n");
            Sb += string.Format("\r\n");
            Sb += string.Format(string.Format("c=1,s={0}\r\n", _tr7Dev.serialNoStr));
            Sb += string.Format("\r\n");
            Sb += string.Format("----boudary_7wf--\r\n");
            contentLength = Sb.Length;

            string Sh = "";
            Sh += string.Format("POST /api/dev-7wf-1.php HTTP/1.0\r\n");
            Sh += string.Format("User-Agent: TandD device (TR-7WF)\r\n");
            Sh += string.Format("Host: develop.webstorage.jp\r\n");
            Sh += string.Format("Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n");
            Sh += string.Format("Connection: keep-alive\r\n");
            Sh += string.Format("Content-Type: multipart/form-data; boundary=--boudary_7wf\r\n");
            Sh += string.Format("Content-Length: {0}\r\n", contentLength);
            Sh += string.Format("\r\n");

            Debug.Write(Sh);
            Debug.Write(Sb);

            rtn = grunt.Open(_tr7Dev.sendToIP, _tr7Dev.sendToPort);
            if (rtn == 0)
            {
                grunt.Send(Sh);
                grunt.Send(Sb);
                Debug.WriteLine("==================================================");

                string rspStr = "";
                byte[] rspBytes = new byte[0];
                grunt.Recv(ref rspBytes, ref rspStr, 2000);

                //testDebug();
                Debug.WriteLine("--------------------------------------------------");
                int idx = rspStr.IndexOf("R=1");
                Debug.WriteLine("idx = {0}, Len = {1}", idx, rspBytes.Length);
                int i;
                for (i = idx + 7; i < rspBytes.Length; i++)
                {
                    Debug.Write(string.Format(" {0:x02}", rspBytes[i]));
                    //Debug.Write(" {0:x02}", rspBytes[i]);
                }
                Debug.WriteLine("");

                int pktLen = rspBytes.Length - (idx + 7);
                byte[] pkt = new byte[pktLen];
                int j = 0;
                for (i = idx + 7; i < rspBytes.Length; i++)
                {
                    pkt[j++] = rspBytes[i];
                }

                UInt16 csA = 0;
                UInt16 csB = 0;
                for (j = 0; j < pktLen; j++)
                {
                    if (j < (pktLen - 2)) csA += pkt[j];
                    if (j == (pktLen - 2)) csB = pkt[j];
                    if (j == (pktLen - 1)) csB += (UInt16)(pkt[j] * 256);
                }
                Debug.WriteLine(string.Format("csA 0x{0:x04}", csA));
                Debug.WriteLine(string.Format("csB 0x{0:x04}", csB));

                Debug.WriteLine("--------------------------------------------------");

                // 6,7,8,9,10,11

                Debug.Write(string.Format("Date: "));
                Debug.Write(string.Format("20{0}", pkt[6]));
                Debug.Write(string.Format("/{0}", pkt[7]));
                Debug.Write(string.Format("/{0}", pkt[8]));
                Debug.Write(string.Format(" {0}:", pkt[9]));
                Debug.Write(string.Format("{0}:", pkt[10]));
                Debug.Write(string.Format("{0}", pkt[11]));


                Debug.WriteLine("");
                Debug.WriteLine("--------------------------------------------------");

                //--------------------------------------------------
                //Date: 2019/3/5 5:43:9
                //--------------------------------------------------
                //now               Nunixtime = 1551764589
                //now                unixtime = 1551764590
                //now                      dt = 2019/03/05 5:43:10 +00:00
                //now                      lt = 2019/03/05 14:43:10 +09:00
                //now                      lt = 2019-03-05 14:43:10             REF:  2018-12-04 08:52:04

                //NOTES: the "recvDT" gives the same as DateTimeOffset.Now
                //long getUnixTimeNow()
                {
                    //現在時刻をUnixTimeへ変換
                    var baseDt = new DateTimeOffset(1970, 1, 1, 0, 0, 0, TimeSpan.Zero);
                    var unixtime = (DateTimeOffset.Now - baseDt).Ticks / 10000000;

                    var recvDT = new DateTimeOffset(2000 + pkt[6], pkt[7], pkt[8], pkt[9], pkt[10], pkt[11], TimeSpan.Zero);
                    var Nunixtime = (recvDT - baseDt).Ticks / 10000000;


                    var now_dt = new DateTimeOffset(unixtime * 10000000 + baseDt.Ticks, TimeSpan.Zero);
                    var now_lt = now_dt.ToLocalTime();
                    Debug.WriteLine(string.Format("now               Nunixtime = {0}", Nunixtime));
                    Debug.WriteLine(string.Format("now                unixtime = {0}", unixtime));
                    Debug.WriteLine(string.Format("now                      dt = {0}", now_dt));
                    Debug.WriteLine(string.Format("now                      lt = {0}", now_lt));

                    StringBuilder sb;
                    string s;
                    s = now_lt.ToString();
                    s = s.Substring(0, 19);
                    sb = new StringBuilder(s);
                    sb[4] = '-';
                    sb[7] = '-';
                    Debug.WriteLine(string.Format("now                      lt = {0}", sb));           //  REF:  2018-12-04 08:52:04

                    //return unixtime;
                }
                //R=1,2
                //--------------------------------------------------
                //idx = 140, Len = 161
                //01 00 6f 00 06 00 12 0c 12 06 2e 01 db 00 //At 2012/12/18 15:46:01
                //	RTC_Struct_t rtc;
                //rtc.Year  = *inpoi++;  12 - 2018
                //rtc.Month = *inpoi++;  0c - 12 (December)
                //rtc.Day   = *inpoi++;  12 - 18th
                //rtc.Hour  = *inpoi++;  06 -  6   6 hour (+9 hours for local time)
                //rtc.Min   = *inpoi++;  2e - 46  46 min
                //rtc.Sec   = *inpoi++;  01 - 01  01 sec
                //                       db - ms
                //                       00 - ms
                //--------------------------------------------------



                /*
                HTTP/1.1 200 OK
                Server: nginx
                Date: Sat, 15 Dec 2018 15:36:07 GMT
                Content-Type: text/html
                Content-Length: 21
                Connection: keep-alive

                R=1,0
                ............
                */
                /*
                HTTP/1.1 200 OK
                Server: nginx
                Date: Sat, 15 Dec 2018 15:36:04 GMT
                Content-Type: text/html
                Content-Length: 7
                Connection: keep-alive

                R=2,0
                */
                grunt.Close();
            }
        }

        private void bt_Test1_Click(object sender, EventArgs e)
        {
            command_c1(tr7DevA);
        }




#endif // 0




