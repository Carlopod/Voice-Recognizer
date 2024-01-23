#include <math.h>
#include <stdio.h>
#include <complex.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <m2mb_ati.h>
#include <m2mb_i2c.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>


#include "types.h"
#include "fft.h"
#include "m2m_log.h"

#include "m2mb_types.h"
#include "m2mb_os_types.h"
#include "m2mb_os_api.h"
#include "m2mb_os.h"
#include "m2mb_os_tmr.h"
#include "m2mb_fs_stdio.h"
#include "m2mb_gpio.h"


#include "azx_log.h"
#include "azx_utils.h"
#include "azx_tasks.h"

#include "app_cfg.h"

#define I2C_GPIO 1

#if I2C_GPIO
#define I2C_SDA 2
#define I2C_SCL 3
#endif

#define DEVICE_ADDR 0x30
#define INTERRUPT_ENABLE_REG 0x04
#define SYS_SHUTDOWN_REG 0x17

#define audioLengthMinusOneSec 32000
#define SEGMENT_SIZE 160  // Lunghezza del segmento audio (es. 10 ms a 16 kHz)
#define ENERGY_THRESHOLD 100000000  // Soglia di energia per il rilevamento vocale

static UINT8 cfg[] = { 0x00, 0x10, 0x90, 0x00, 0x10, 0x0A, 0x33, 0x00,
		0x00, 0x33, 0x0C, 0x0C, 0x09, 0x09, 0x24, 0x24, 0x40, 0x00, 0x60};

#define BUFSIZE 128


#define FILTER_DIM 3
#define F_DIM_MINUS1 2
#define F_DIM_MINUS2 1
#define F_DIM_MINUS3 0
#define TIMER2 1
#define MAX_LENGTH 36

#define CIFRE
//#define calcoloSoglia




extern INT32 gpio_fd_1;


const float mean = 4.7;
UINT32 nitems;

typedef struct bark_band_{
	UINT8 start_freq; //posizione di ZPower dala quale comincier� a moltiplicare per length valori
	UINT8 length; //numero di valori da leggere per il singolo array
	float band_filter[MAX_LENGTH];
}bark_band;

const bark_band filters[BARK_BANDS]={
		{1,2,{0.0227854056313136,0.00861796826820540,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{2,2,{0.0174580101798678,0.0134011198040602,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{3,2,{0.0118881583945353,0.0183582552148527,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{4,3,{0.00617697412470797,0.0233946406509205,0.00599679691455663,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{5,3,{0.000446318911454656,0.0183664303685998,0.0115522278690299,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{7,2,{0.0136012584161902,0.0171362627992139,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{8,3,{0.00835854687117434,0.0219086596455560,0.00257818804392388,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{9,3,{0.00307064614902871,0.0217182084488081,0.00862072469613578,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{11,2,{0.0150241121992536,0.0146366218082942,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{12,3,{0.00837963628466444,0.0205590449846567,0.00505829434111389,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{13,3,{0.00184951625254499,0.0167720031402113,0.0119671578393691,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{15,3,{0.00930666538785514,0.0186046830848026,0.00531995465259385,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{16,4,{0.00212840672724702,0.0148983308362667,0.0126555052765062,0.000385132210868535,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{18,3,{0.00706590815480887,0.0188513867585846,0.00826121209952802,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{20,3,{0.0105182390748566,0.0156348215723223,0.00521784124768778,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{21,4,{0.00269590494676095,0.0126842898627754,0.0129093981859775,0.00333610978145128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{23,4,{0.00457733218822795,0.0137484619922971,0.0112228470800906,0.00244093796544759,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{25,4,{0.00547606694377462,0.0138811441979838,0.0104116073287536,0.00237097310598610,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{27,4,{0.00555114579536912,0.0132391886377104,0.0103254165015038,0.00297811338857329,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{29,4,{0.00494849729452726,0.0119663870551478,0.0108268345008414,0.00412703171543016,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{31,4,{0.00380141812479550,0.0101939469422456,0.0117907673997460,0.00569469954300886,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{33,5,{0.00223104926423375,0.00804096770171564,0.0131039912791943,0.00756991071951000,0.00203583015982575,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{35,5,{0.000346852147898512,0.00561491680951421,0.0108829814711299,0.00965280532239957,0.00464093457912335,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{38,5,{0.00301210597732544,0.00777712712432499,0.0118543934959262,0.00732687762314047,0.00279936175035479,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{40,6,{0.000318167315044151,0.00461705648207440,0.00891594564910464,0.0100169382589045,0.00593779722917125,0.00185865619943800,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{43,6,{0.00147634710461145,0.00534416471596390,0.00921198232731635,0.00897934666712308,0.00531442775523523,0.00164950884334738,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{46,6,{0.00188889987745376,0.00535890286331304,0.00882890584917233,0.00858526297260712,0.00530219313934038,0.00201912330607364,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{49,6,{0.00170677906997403,0.00481046848266615,0.00791415789535828,0.00869422512317020,0.00576236339903475,0.00283050167489931,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{52,7,{0.00106533012027656,0.00383249875051726,0.00659966738075796,0.00918095051868996,0.00657134038768207,0.00396173025667419,0.00135212012566631,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{55,8,{8.50047118914801e-05,0.00254378470503451,0.00500256469817755,0.00746134469132058,0.00761994355073722,0.00530526533409107,0.00299058711744492,0.000675908900798773,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{59,8,{0.00104898548978464,0.00322589596207000,0.00540280643435536,0.00757971690664072,0.00676721892196119,0.00472174216190053,0.00267626540183986,0.000630788641779196,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{63,8,{0.00135933725225961,0.00327933190706190,0.00519932656186419,0.00711932121666647,0.00646618036697967,0.00466571621046945,0.00286525205395924,0.00106478789744902,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{67,9,{0.00116642360174314,0.00285293834581985,0.00453945308989657,0.00622596783397329,0.00657853005154836,0.00500038363404657,0.00342223721654479,0.00184409079904301,0.000265944381541222,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{71,10,{0.000602690639510316,0.00207769119925011,0.00355269175898990,0.00502769231872969,0.00650269287846948,0.00560623907680449,0.00422916190601375,0.00285208473522302,0.00147500756443228,9.79303936415379e-05,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{76,10,{0.00106764882832142,0.00235167836599246,0.00363570790366350,0.00491973744133453,0.00620376697900557,0.00518426568749299,0.00398840802711230,0.00279255036673161,0.00159669270635093,0.000400835045970242,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{81,11,{0.00103313108902926,0.00214535781902745,0.00325758454902563,0.00436981127902381,0.00548203800902200,0.00516799760332945,0.00413486085680593,0.00310172411028240,0.00206858736375887,0.00103545061723534,2.31387071181029e-06,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{86,12,{0.000637067617565331,0.00159533242266264,0.00255359722775994,0.00351186203285725,0.00447012683795455,0.00542839164305186,0.00454491486358357,0.00365730395786405,0.00276969305214454,0.00188208214642503,0.000994471240705517,0.000106860334986005,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{92,12,{0.000819025594663404,0.00163988964225619,0.00246075368984898,0.00328161773744177,0.00410248178503457,0.00492334583262736,0.00435540548803474,0.00359738125731760,0.00283935702660046,0.00208133279588332,0.00132330856516618,0.000565284334449039,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{98,13,{0.000614664070817610,0.00131345642917489,0.00201224878753217,0.00271104114588946,0.00340983350424674,0.00410862586260402,0.00446620302920146,0.00382303459868824,0.00317986616817503,0.00253669773766181,0.00189352930714859,0.00125036087663538,0.000607192446122165,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{104,15,{0.000150237028380051,0.000741102282891783,0.00133196753740352,0.00192283279191525,0.00251369804642698,0.00310456330093871,0.00369542855545044,0.00422293291492017,0.00368105008456734,0.00313916725421450,0.00259728442386167,0.00205540159350883,0.00151351876315599,0.000971635932803158,0.000429753102450323,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{111,17,{2.77410084369606e-05,0.000523686877862363,0.00101963274728777,0.00151557861671317,0.00201152448613857,0.00250747035556397,0.00300341622498938,0.00349936209441478,0.00379893499747248,0.00334588062574305,0.00289282625401362,0.00243977188228419,0.00198671751055476,0.00153366313882533,0.00108060876709589,0.000627554395366462,0.000174500023637030,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{119,18,{8.54491092780995e-05,0.000498394061993958,0.000911339014709817,0.00132428396742568,0.00173722892014153,0.00215017387285739,0.00256311882557325,0.00297606377828911,0.00338900873100497,0.00331711666424089,0.00294149905185620,0.00256588143947152,0.00219026382708684,0.00181464621470215,0.00143902860231747,0.00106341098993278,0.000687793377548099,0.000312175765163415,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{128,20,{0.000209549098855560,0.000550369969104485,0.000891190839353409,0.00123201170960233,0.00157283257985126,0.00191365345010018,0.00225447432034911,0.00259529519059803,0.00293611606084696,0.00316725733666880,0.00285870261036022,0.00255014788405164,0.00224159315774306,0.00193303843143447,0.00162448370512589,0.00131592897881731,0.00100737425250873,0.000698819526200151,0.000390264799891570,8.17100735829892e-05,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{137,22,{4.70521192351121e-05,0.000325631723373862,0.000604211327512613,0.000882790931651363,0.00116137053579011,0.00143995013992886,0.00171852974406761,0.00199710934820636,0.00227568895234511,0.00255426855648387,0.00283284816062262,0.00272216587138381,0.00247127036764438,0.00222037486390495,0.00196947936016552,0.00171858385642608,0.00146768835268665,0.00121679284894722,0.000965897345207790,0.000715001841468358,0.000464106337728926,0.000213210833989495,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
		{148,24,{0.000165618506857056,0.000390893259604841,0.000616168012352625,0.000841442765100410,0.00106671751784819,0.00129199227059598,0.00151726702334376,0.00174254177609155,0.00196781652883933,0.00219309128158712,0.00241836603433490,0.00257950629211601,0.00237778894095220,0.00217607158978839,0.00197435423862458,0.00177263688746077,0.00157091953629696,0.00136920218513315,0.00116748483396934,0.000965767482805526,0.000764050131641716,0.000562332780477906,0.000360615429314095,0.000158898078150285,0,0,0,0,0,0,0,0,0,0,0,0}},
		{159,28,{2.70372495534111e-05,0.000207044780240161,0.000387052310926911,0.000567059841613661,0.000747067372300410,0.000927074902987160,0.00110708243367391,0.00128708996436066,0.00146709749504741,0.00164710502573416,0.00182711255642091,0.00200712008710766,0.00218712761779441,0.00229492967647962,0.00213478438516302,0.00197463909384642,0.00181449380252981,0.00165434851121321,0.00149420321989661,0.00133405792858000,0.00117391263726340,0.00101376734594680,0.000853622054630192,0.000693476763313588,0.000533331471996985,0.000373186180680381,0.000213040889363778,5.28955980471744e-05,0,0,0,0,0,0,0,0}},
		{172,31,{3.01273094592057e-05,0.000172054078273889,0.000313980847088573,0.000455907615903256,0.000597834384717940,0.000739761153532623,0.000881687922347307,0.00102361469116199,0.00116554145997667,0.00130746822879136,0.00144939499760604,0.00159132176642072,0.00173324853523541,0.00187517530405009,0.00201710207286478,0.00198003162116608,0.00185467965798509,0.00172932769480410,0.00160397573162310,0.00147862376844211,0.00135327180526112,0.00122791984208013,0.00110256787889914,0.000977215915718152,0.000851863952537162,0.000726511989356171,0.000601160026175181,0.000475808062994191,0.000350456099813200,0.000225104136632210,9.97521734512193e-05,0,0,0,0,0}},
		{187,33,{7.89340585676261e-05,0.000196798431124462,0.000314662803681297,0.000432527176238133,0.000550391548794968,0.000668255921351803,0.000786120293908639,0.000903984666465474,0.00102184903902231,0.00113971341157915,0.00125757778413598,0.00137544215669282,0.00149330652924965,0.00161117090180649,0.00172903527436332,0.00184689964692016,0.00191669115509163,0.00179916187832362,0.00168163260155562,0.00156410332478761,0.00144657404801961,0.00132904477125160,0.00121151549448360,0.00109398621771559,0.000976456940947588,0.000858927664179583,0.000741398387411578,0.000623869110643574,0.000506339833875569,0.000388810557107564,0.000271281280339559,0.000153752003571554,3.62227268035497e-05,0,0,0}},
		{203,34,{2.34615891231476e-05,0.000138343636474972,0.000253225683826797,0.000368107731178622,0.000482989778530447,0.000597871825882272,0.000712753873234097,0.000827635920585922,0.000942517967937747,0.00105740001528957,0.00117228206264140,0.00128716410999322,0.00140204615734505,0.00151692820469687,0.00163181025204870,0.00174669229940052,0.00186157434675235,0.00182079549694614,0.00171066845619818,0.00160054141545022,0.00149041437470226,0.00138028733395430,0.00127016029320634,0.00116003325245838,0.00104990621171042,0.000939779170962465,0.000829652130214505,0.000719525089466545,0.000609398048718585,0.000499271007970626,0.000389143967222666,0.000279016926474706,0.000168889885726746,5.87628449787862e-05,0,0}},
		{220,36,{7.00521373723869e-05,0.000171313069548563,0.000272574001724739,0.000373834933900915,0.000475095866077091,0.000576356798253267,0.000677617730429443,0.000778878662605619,0.000880139594781795,0.000981400526957971,0.00108266145913415,0.00118392239131032,0.00128518332348650,0.00138644425566268,0.00148770518783885,0.00158896612001503,0.00169022705219120,0.00170246716096272,0.00161286362617521,0.00152326009138770,0.00143365655660018,0.00134405302181267,0.00125444948702516,0.00116484595223765,0.00107524241745014,0.000985638882662625,0.000896035347875113,0.000806431813087602,0.000716828278300090,0.000627224743512578,0.000537621208725066,0.000448017673937554,0.000358414139150042,0.000268810604362530,0.000179207069575019,8.96035347875066e-05}}
};

const float hann[DATA_LEN]={
		0,6.16837591697061e-05,0.000246719817134200,0.000555062519014993,0.000986635785864221,0.00154133313343602,0.00221901769846000,0.00301952227241015,0.00394264934276106,0.00498817114172123,0.00615582970243112,0.00744533692261307,0.00885637463565564,0.0103885946891171,0.0120416190306263,0.0138150398011617,0.0157084194356845,0.0177212907711010,0.0198531571615285,0.0221034926008349,0.0244717418524232,0.0269573205862274,0.0295596155228873,0.0322779845850664,0.0351117570558743,0.0380602337443566,0.0411226871580094,0.0442983616822774,0.0475864737669903,0.0509862121196922,0.0544967379058161,0.0581171849556533,0.0618466599780682,0.0656842427809044,0.0696289864980282,0.0736799178229539,0.0778360372489925,0.0820963193158649,0.0864597128627191,0.0909251412874883,0.0954915028125263,0.100157670756455,0.104922493812155,0.109784796330835,0.114743378612105,0.119797017199985,0.124944465184770,0.130184452510695,0.135515686289294,0.140936851118406,0.146446609406726,0.152043601703843,0.157726447035656,0.163493743245113,0.169344067338174,0.175275975834908,0.181288005125655,0.187378671832147,0.193546473173512,0.199789887337058,0.206107373853763,0.212497373978361,0.218958311073935,0.225488591000934,0.232086602510502,0.238750717642026,0.245479292124814,0.252270665783796,0.259123162949142,0.266035092869713,0.273004750130227,0.280030415072042,0.287110354217464,0.294242820697446,0.301426054682610,0.308658283817455,0.315937723657661,0.323262578110371,0.330631039877354,0.338041290900925,0.345491502812526,0.352979837383848,0.360504446980385,0.368063475017314,0.375655056417573,0.383277318072047,0.390928379301729,0.398606352321744,0.406309342707138,0.414035449860295,0.421782767479885,0.429549384031209,0.437333383217848,0.445132844454477,0.452945843340743,0.460770452136078,0.468604740235343,0.476446774645179,0.484294620460936,0.492146341344090,0.500000000000000,0.507853658655910,0.515705379539064,0.523553225354821,0.531395259764657,0.539229547863922,0.547054156659257,0.554867155545523,0.562666616782152,0.570450615968791,0.578217232520116,0.585964550139705,0.593690657292862,0.601393647678256,0.609071620698271,0.616722681927953,0.624344943582427,0.631936524982686,0.639495553019615,0.647020162616152,0.654508497187474,0.661958709099075,0.669368960122646,0.676737421889629,0.684062276342339,0.691341716182545,0.698573945317390,0.705757179302554,0.712889645782536,0.719969584927958,0.726995249869773,0.733964907130287,0.740876837050858,0.747729334216204,0.754520707875186,0.761249282357974,0.767913397489499,0.774511408999066,0.781041688926065,0.787502626021639,0.793892626146237,0.800210112662942,0.806453526826488,0.812621328167853,0.818711994874345,0.824724024165092,0.830655932661826,0.836506256754887,0.842273552964344,0.847956398296157,0.853553390593274,0.859063148881594,0.864484313710706,0.869815547489305,0.875055534815230,0.880202982800015,0.885256621387895,0.890215203669165,0.895077506187845,0.899842329243545,0.904508497187474,0.909074858712512,0.913540287137281,0.917903680684135,0.922163962751007,0.926320082177046,0.930371013501972,0.934315757219096,0.938153340021932,0.941882815044347,0.945503262094184,0.949013787880308,0.952413526233010,0.955701638317723,0.958877312841991,0.961939766255643,0.964888242944126,0.967722015414934,0.970440384477113,0.973042679413773,0.975528258147577,0.977896507399165,0.980146842838472,0.982278709228899,0.984291580564316,0.986184960198838,0.987958380969374,0.989611405310883,0.991143625364344,0.992554663077387,0.993844170297569,0.995011828858279,0.996057350657239,0.996980477727590,0.997780982301540,0.998458666866564,0.999013364214136,0.999444937480985,0.999753280182866,0.999938316240830,1,0.999938316240830,0.999753280182866,0.999444937480985,0.999013364214136,0.998458666866564,0.997780982301540,0.996980477727590,0.996057350657239,0.995011828858279,0.993844170297569,0.992554663077387,0.991143625364344,0.989611405310883,0.987958380969374,0.986184960198838,0.984291580564316,0.982278709228899,0.980146842838472,0.977896507399165,0.975528258147577,0.973042679413773,0.970440384477113,0.967722015414934,0.964888242944126,0.961939766255643,0.958877312841991,0.955701638317723,0.952413526233010,0.949013787880308,0.945503262094184,0.941882815044347,0.938153340021932,0.934315757219096,0.930371013501972,0.926320082177046,0.922163962751007,0.917903680684135,0.913540287137281,0.909074858712512,0.904508497187474,0.899842329243545,0.895077506187845,0.890215203669165,0.885256621387895,0.880202982800015,0.875055534815230,0.869815547489305,0.864484313710706,0.859063148881594,0.853553390593274,0.847956398296157,0.842273552964344,0.836506256754887,0.830655932661826,0.824724024165092,0.818711994874345,0.812621328167853,0.806453526826488,0.800210112662942,0.793892626146237,0.787502626021639,0.781041688926065,0.774511408999066,0.767913397489499,0.761249282357974,0.754520707875186,0.747729334216204,0.740876837050858,0.733964907130287,0.726995249869773,0.719969584927958,0.712889645782536,0.705757179302554,0.698573945317390,0.691341716182545,0.684062276342339,0.676737421889629,0.669368960122646,0.661958709099075,0.654508497187474,0.647020162616152,0.639495553019615,0.631936524982686,0.624344943582427,0.616722681927953,0.609071620698271,0.601393647678256,0.593690657292862,0.585964550139705,0.578217232520116,0.570450615968791,0.562666616782152,0.554867155545523,0.547054156659257,0.539229547863922,0.531395259764657,0.523553225354821,0.515705379539064,0.507853658655910,0.500000000000000,0.492146341344090,0.484294620460936,0.476446774645179,0.468604740235343,0.460770452136078,0.452945843340743,0.445132844454477,0.437333383217848,0.429549384031209,0.421782767479885,0.414035449860295,0.406309342707138,0.398606352321744,0.390928379301729,0.383277318072047,0.375655056417573,0.368063475017314,0.360504446980385,0.352979837383848,0.345491502812526,0.338041290900925,0.330631039877354,0.323262578110371,0.315937723657661,0.308658283817455,0.301426054682610,0.294242820697446,0.287110354217464,0.280030415072042,0.273004750130227,0.266035092869713,0.259123162949142,0.252270665783796,0.245479292124814,0.238750717642026,0.232086602510502,0.225488591000934,0.218958311073935,0.212497373978361,0.206107373853763,0.199789887337058,0.193546473173512,0.187378671832147,0.181288005125655,0.175275975834908,0.169344067338174,0.163493743245113,0.157726447035656,0.152043601703843,0.146446609406726,0.140936851118406,0.135515686289294,0.130184452510695,0.124944465184770,0.119797017199985,0.114743378612105,0.109784796330835,0.104922493812155,0.100157670756455,0.0954915028125263,0.0909251412874883,0.0864597128627191,0.0820963193158649,0.0778360372489925,0.0736799178229539,0.0696289864980282,0.0656842427809044,0.0618466599780682,0.0581171849556533,0.0544967379058161,0.0509862121196922,0.0475864737669903,0.0442983616822774,0.0411226871580094,0.0380602337443566,0.0351117570558743,0.0322779845850664,0.0295596155228873,0.0269573205862274,0.0244717418524232,0.0221034926008349,0.0198531571615285,0.0177212907711010,0.0157084194356845,0.0138150398011617,0.0120416190306263,0.0103885946891171,0.00885637463565564,0.00744533692261307,0.00615582970243112,0.00498817114172123,0.00394264934276106,0.00301952227241015,0.00221901769846000,0.00154133313343602,0.000986635785864221,0.000555062519014993,0.000246719817134200,6.16837591697061e-05
};

const float PI = 3.14159265358979323846264L;

const float conv_fact = 0.000030517578125; // 1/(2^(16-1)) : 1/max int rappresentabile con 16 bit

const INT16 null_tail[FIN_FFT-DATA_LEN] = {0};



void _fft_p(cplx* buf, cplx* out, int n, int step){
	int i=0;
	if (step < n) {
		_fft_p(out, buf, n, step * 2);
		_fft_p(out + step, buf + step, n, step * 2);

		for (i = 0; i < n; i += 2 * step) {
			cplx t = cexp(-I * PI * i / n) * (*(out+ i+ step));

			*(buf+(i/2)) = *(out+i) +t;
			*(buf+((i+n)/2)) = *(out+i) -t;
		}
	}
}

void fft_p(cplx* buf, int n){
	static cplx out[512];
	memcpy(out, buf, sizeof(out));
	_fft_p(buf, out, 512, 1);
}

/*
 * ((float) *(param1 + i))*conv_fact : casto a float e ne cambio la scala col fattore di convergenza
 * 		qui sto effettivamente copiando dei valori:
 * 			fft ï¿½ un puntatore a elementi cplx;
 * 			*fft ï¿½ il valore cplx salvato all'indirizzo fft.
 * 		(sommando +i mi sposto di indice: mi sposto di (i*sizeof(cplx)) bytes)
 */
void int2cplx(INT16* param1, cplx* fft, int dim){
	int i=0;
	for(i=0;i<dim;i++){
		*(fft+i) =((float) *(param1 + i))*conv_fact;
	}
}


INT16 *paramArr[98]={NULL};


/*
 * con ciascun puntatore passato come parametro, riempie mano mano paramArr
 * static int arrIndex : mettendo la variabile a static, essa rimane in vita per tutta la durata dell'esecuzione
 * paramArr[arrIndex++] : avendo la variabile come static, con ++, DOPO ogni associazione, arrIndex aumenta
 * 		avessi fatto paramArr[++arrIndex], arrIndex sarebbe aumentato PRIMA di effettuare l'associazione
 * if(arrIndex >= 98) : superata la dimensione massima dell'array, riparto dall'inizio sovrascrivendolo a partire dalla prima posizione
 */
void setParam(INT16* p_address)
{
	static int arrIndex = 0;
	paramArr[arrIndex++] = p_address;
	//AZX_LOG_INFO("%p \r\n", paramArr[arrIndex -1] );
	if(arrIndex >= 98)
	{
		arrIndex=0;
	}
}

/*
 * legge i dati dall'array di puntatori riempito con setParam
 */
INT16* getParam(void)
{
	static int flux_counter = 0;
	if(flux_counter >= 98)
	{
		flux_counter = 0;
	}
	return paramArr[flux_counter++];
}

M2MB_OS_TMR_ATTR_HANDLE tmrAttrHandle;
M2MB_OS_TMR_HANDLE      tmrHandle;
M2MB_OS_RESULT_E        osRes;
M2MB_OS_RESULT_E        memfRes;

void timeout_handler( M2MB_OS_TMR_HANDLE internTmrHandle, void *ctx)
{
	(void)internTmrHandle;
	static int counter = 0;
	if(counter==0 ){
		azx_tasks_sendMessageToTask(1, SEND, (INT32) getParam() , 400*2);
	}
	else{
		azx_tasks_sendMessageToTask(1, SEND, (INT32) getParam() , NHOP*2);
	}
	counter++;
	if(counter >=98){
		counter=0;
	}

}

void on_timer_action(void){
	M2M_LOG_INFO("Creazione del timer da 5ms!\r\n");

	/* Create the attribute structure which will hold the timer attributes */
	if ( m2mb_os_tmr_setAttrItem( &tmrAttrHandle, 1, M2MB_OS_TMR_SEL_CMD_CREATE_ATTR, NULL ) != M2MB_OS_SUCCESS )
	{
		AZX_LOG_ERROR("error_creating attribute timer\r\n");
		return;
	}

	/* set the attributes in parameters structure */
	osRes = m2mb_os_tmr_setAttrItem( &tmrAttrHandle,
			CMDS_ARGS(
					M2MB_OS_TMR_SEL_CMD_NAME, "mytmr",  /*optional timer name */
					M2MB_OS_TMR_SEL_CMD_CB_FUNC, &timeout_handler, /*the callback function to run */
					M2MB_OS_TMR_SEL_CMD_ARG_CB, &tmrHandle, /*arg for callback function, here handle of timer for example*/
					M2MB_OS_TMR_SEL_CMD_TICKS_PERIOD, M2MB_OS_MS2TICKS( 4 ),  /*wait 20 milliseconds */
					M2MB_OS_TMR_SEL_CMD_PERIODIC, M2MB_OS_TMR_PERIODIC_TMR  /*set the timer as periodic (auto restarts after expiration)*/
			)
	);

	if ( osRes != M2MB_OS_SUCCESS )
	{
		/* delete the attributes structure in case of failure*/
		m2mb_os_tmr_setAttrItem( &tmrAttrHandle, 1, M2MB_OS_TMR_SEL_CMD_DEL_ATTR, NULL );
		AZX_LOG_ERROR("error setting or creating tmrAttrHandle\r\n");
		return;
	}


	/* Now, init timer passing the attributes structure */
	if ( m2mb_os_tmr_init( &tmrHandle, &tmrAttrHandle ) != M2MB_OS_SUCCESS )
	{
		/* in case of error, manually remove attributes structure and release resources (in case of success, it will be managed by the task deinit function*/
		m2mb_os_tmr_setAttrItem( &tmrAttrHandle, 1, M2MB_OS_TMR_SEL_CMD_DEL_ATTR, NULL );
	}
}

void timer_stop(void){
	osRes = m2mb_os_tmr_stop(tmrHandle);
	if( osRes != M2MB_OS_SUCCESS )
	{
		AZX_LOG_ERROR("Cannot stop running timer! Res: %d\r\n", osRes);
	}
}

void communication_error( M2MB_OS_TMR_HANDLE tmrHandle)
{
	(void)tmrHandle;
	AZX_LOG_INFO("Task1 rimasto in attesa troppo a lungo, effettuo restart.\r\n");
	//	m2m_os_send_message_to_task(1,OK,0,0);
	m2mb_os_tmr_stop(tmrHandle);
}

void restart_w_new_param(void){
	MEM_W currDuration = 20000;
	m2mb_os_tmr_setItem( tmrHandle, M2MB_OS_TMR_SEL_CMD_TICKS_PERIOD, (void*)currDuration );
	m2mb_os_tmr_setItem( tmrHandle, M2MB_OS_TMR_SEL_CMD_CB_FUNC, (void*)communication_error );
	m2mb_os_tmr_start(tmrHandle);
}

void timer_start(void){
	m2mb_os_tmr_start(tmrHandle);
}


M2MB_OS_TMR_ATTR_HANDLE tmr2AttrHandle;
M2MB_OS_TMR_HANDLE      tmr2Handle;
M2MB_OS_RESULT_E        os2Res;

M2MB_GPIO_VALUE_E value_read;

void timeout2_handler( M2MB_OS_TMR_HANDLE tmrHandle, void *ctx){
	(void)tmr2Handle;
	INT32 ret;

	ret = m2mb_gpio_read(gpio_fd_1, &value_read );
	if ( ret  ) {
		AZX_LOG_ERROR("gpio read failed!\r\n");
		m2mb_os_taskSleep( M2MB_OS_MS2TICKS( 500 ) );
		return;
	}

	if(value_read == 0){
		UINT32 arg_value = 0;
		m2mb_gpio_ioctl( gpio_fd_1, M2MB_GPIO_IOCTL_DEINIT_INTR,  arg_value );
		m2mb_gpio_close(gpio_fd_1);
		M2M_LOG_INFO("Recording\r\n");
		azx_tasks_sendMessageToTask(1, TIMER, 0, 0);
	}
	else{ M2M_LOG_INFO("Click for at least half a second\r\n\r\n");
	}
}


void on_timer2_action(void){
	M2M_LOG_INFO("Creazione del timer da 500ms!\r\n");

	/* Create the attribute structure which will hold the timer attributes */
	if ( m2mb_os_tmr_setAttrItem( &tmr2AttrHandle, 1, M2MB_OS_TMR_SEL_CMD_CREATE_ATTR, NULL ) != M2MB_OS_SUCCESS )
	{
		AZX_LOG_ERROR("error_creating attribute timer\r\n");
		return;
	}

	/* set the attributes in parameters structure */
	osRes = m2mb_os_tmr_setAttrItem( &tmr2AttrHandle,
			CMDS_ARGS(
					M2MB_OS_TMR_SEL_CMD_NAME, "my2tmr",  /*optional timer name */
					M2MB_OS_TMR_SEL_CMD_CB_FUNC, &timeout2_handler, /*the callback function to run */
					M2MB_OS_TMR_SEL_CMD_ARG_CB, &tmr2Handle, /*arg for callback function, here handle of timer for example*/
					M2MB_OS_TMR_SEL_CMD_TICKS_PERIOD, M2MB_OS_MS2TICKS( 1000 ),  /*wait 200 milliseconds */
					M2MB_OS_TMR_SEL_CMD_PERIODIC, M2MB_OS_TMR_ONESHOT_TMR  /*set the timer as periodic (auto restarts after expiration)*/
			)
	);

	if ( osRes != M2MB_OS_SUCCESS )
	{
		/* delete the attributes structure in case of failure*/
		m2mb_os_tmr_setAttrItem( &tmr2AttrHandle, 1, M2MB_OS_TMR_SEL_CMD_DEL_ATTR, NULL );
		AZX_LOG_ERROR("error setting or creating tmr2AttrHandle\r\n");
		return;
	}

	AZX_LOG_INFO("Start timer 2\r\n");
	/* Now, init timer passing the attributes structure */
	if ( m2mb_os_tmr_init( &tmr2Handle, &tmr2AttrHandle ) != M2MB_OS_SUCCESS )
	{
		/* in case of error, manually remove attributes structure and release resources (in case of success, it will be managed by the task deinit function*/
		m2mb_os_tmr_setAttrItem( &tmr2AttrHandle, 1, M2MB_OS_TMR_SEL_CMD_DEL_ATTR, NULL );
	}
	/*Now timer can be started. Once expired, the callback function will be executed*/
	//m2mb_os_tmr_start(tmr2Handle);
}

void timer2_stop(void){
	//		AZX_LOG_INFO("Stopping the timer2\r\n");
	os2Res = m2mb_os_tmr_stop(tmr2Handle);
	if( os2Res != M2MB_OS_SUCCESS )
	{
		AZX_LOG_ERROR("Cannot stop running timer! Res: %d\r\n", os2Res);
	}
	else
	{
		//AZX_LOG_INFO("Stop a running timer2: success\r\n" );
	}
}


void timer2_start(void){
	m2mb_os_tmr_start(tmr2Handle);
}


/*
 * Funzione di debug che permette di formattare la stampa dell'uscita sulla base del tipo
 * di elemento contenuto nell'array il cui indirizzo passato come parametro arr_address
 */
void general_printer(const INT32 arr_address, int print_len, type_switcher print_type){
	int i=0;
	M2M_LOG_INFO("CHECKER =[");

	switch(print_type){

	case P_DOUBLE:{
		float *d_pointer = (float* ) arr_address;
		for(i=0; i < print_len; i++)
		{
			M2M_LOG_INFO("%g ", *(d_pointer+i) );
		}
	}break;

	case P_INT_16:{
		INT16 *int_pointer = (INT16* ) arr_address;
		for(i=0; i < print_len; i++)
		{
			M2M_LOG_INFO("%d ", *(int_pointer + i));
		}
	}break;

	case P_CPLX:{
		cplx *cplx_pointer = (cplx* ) arr_address;
		for(i=0; i < print_len; i++)
		{
			if (!cimag( *(cplx_pointer + i) ))
				M2M_LOG_INFO("%g ", creal( *(cplx_pointer + i) ));
			else
				M2M_LOG_INFO("(%g, %g) ", creal( *(cplx_pointer + i) ), cimag( *(cplx_pointer + i) ));
		}
	}break;

	default:
		break;
	}
	M2M_LOG_INFO("];\n");
}

void error_num_handler(INT32 errNum){
	switch(errNum){
	case BUF_ALLOC_ERROR:
		M2M_LOG_ERROR("BUF_ALLOC_ERROR");
		break;
	case NOT_INIT_ERROR:
		M2M_LOG_ERROR("NOT_INIT_ERROR");
		break;
	case USB_CABLE_NOT_PLUG:
		M2M_LOG_ERROR("USB_CABLE_NOT_PLUG");
		break;
	case NO_MORE_USB_INSTANCE:
		M2M_LOG_ERROR("NO_MORE_USB_INSTANCE");
		break;
	case CANNOT_OPEN_USB_CH:
		M2M_LOG_ERROR("CANNOT_OPEN_USB_CH");
		break;
	case MAX_ERROR_LEN:
		M2M_LOG_ERROR("MAX_ERROR_LEN");
		break;
	default:
		break;
	}
}

/*
 * funzione che stampa la matrice bidimensionale da dare al matlab:
 * 		non stampa precisamente una matrice bidimensionale ma stampa i singoli array che la costituiscono
 * 		cambiando la scala con cui sono espressi i singoli valori (passa in scala logaritmica)
 */
void bid_spec_print (  float ***input, float *f, int width, float iminput)
{
	float epsil = 1e-6;
	int m;
	static int i;

	for(m=0; m < width; m++){
		f[m] = (log10(f[m]+epsil));
		input[0][i][m] =  f[m] - iminput;
	}
	i+=1;
	if(i >=98){
		i=0;
	}
}

/*
 * Funzione che esegue una produttoria per applicare il filtro di hann all'arrray di ingresso
 */
void hann_window_p(cplx* xin){
	int j=0;
	for(j=0;j<DATA_LEN;j++){
		*(xin+j)=(*(xin+j))*hann[j];
	}
}

/*
 * Converte gli array da array di numeri complessi ad array di valori float rappresentati secondo la scala Bark:
 * 	applica i filtri triangolari
 */
float* bark_spec_array(cplx *fArr){
	float * array_spec;
	array_spec = m2mb_os_malloc( (BARK_BANDS+1)*sizeof(float) );
	int i,j=0;
	float sum;
	for(j=0; j<BARK_BANDS; j++){
		sum=0.0;
		for(i=0; i<filters[j].length; i++){
			sum=sum+( filters[j].band_filter[i]* (creal( *(fArr+i+filters[j].start_freq)*conj(*(fArr+i+filters[j].start_freq)) )) );
		}
		*(array_spec+j)=sum;
	}
	return array_spec;
}

#ifdef CIFRE
void matrix_print ( float ***input){
	int i,j,k;
	int y = 7;
	int x = 13;
	int z = 1;
	int riga = 0;
	float max = 0;

	char numero[13];
	char output[14*50]= "";
	for(k = 0; k < z; k++){
		for(i = 1; i < x; i++)
		{
			output[0] = '\0';
			for(j = 0; j < y; j++){
				sprintf(numero, "%g ", input[k][i][j]);
				strcat(output, numero);
				if(input[k][i][j] > max){
					max = input[k][i][j];
					riga = i;
				}
			}
			//M2M_LOG_INFO("%d\r\n",i);
			//M2M_LOG_INFO("%s\r\n" , output);

		}
	}
	switch (riga) {
	case 1:
		M2M_LOG_INFO("The recognised word is BACKGROUND with a success probability of %f\r\n",max);
		break;
	case 2:
		M2M_LOG_INFO("The recognised word is EIGHT with a success probability of %f\r\n",max);
		break;
	case 3:
		M2M_LOG_INFO("The recognised word is FIVE with a success probability of %f\r\n",max);
		break;
	case 4:
		M2M_LOG_INFO("The recognised word is FOUR with a success probability of %f\r\n",max);
		break;
	case 5:
		M2M_LOG_INFO("The recognised word is NINE with a success probability of %f\r\n",max);
		break;
	case 6:
		M2M_LOG_INFO("The recognised word is ONE with a success probability of %f\r\n",max);
		break;
	case 7:
		M2M_LOG_INFO("The recognised word is SEVEN with a success probability of %f\r\n",max);
		break;
	case 8:
		M2M_LOG_INFO("The recognised word is SIX with a success probability of %f\r\n",max);
		break;
	case 9:
		M2M_LOG_INFO("The recognised word is THREE with a success probability of %f\r\n",max);
		break;
	case 10:
		M2M_LOG_INFO("The recognised word is TWO with a success probability of %f\r\n",max);
		break;
	case 11:
		M2M_LOG_INFO("The recognised word is ZERO with a success probability of %f\r\n",max);
		break;
	case 12:
		M2M_LOG_INFO("The recognised word is  UNKNOWN with a success probability of %f\r\n",max);
		break;
	default:
		M2M_LOG_INFO("ERROR N THE SWITCH CASE\r\n",max);
		break;
	}
}
#endif

void filter_print ( float ****input){
	int i,j,l;
	int y = 3;
	int x = 3;
	int w = 3;

	char numero[13];
	char output[14*3]= "";
	for(l = 0; l < w; l++){
		for(i = 0; i < x; i++)
		{
			output[0] = '\0';
			for(j = 0; j < y; j++){
				sprintf(numero, "%g ", input[l][0][i][j]);
				strcat(output, numero);

			}
			M2M_LOG_INFO("%s\r\n" , output);
		}
	}
}



void convolution(float ***input, float ***output, float ****filter, int height, int width, int depth, int FILTER_CHANNEL, float* bias, int indicek) {
	int i, j, k, z;
	float sum;

	//output(i,j) = SUMx SUMy filter(x,y) * input(i-x,j-y)
	// Ciclo attraverso tutti gli elementi (i,j) della matrice output
	for (k = 0; k < depth; k++) {
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				sum = 0.0;

				// Ciclo attraverso tutti gli elementi (x,y) del filtro
				for(z = 0; z < FILTER_CHANNEL; z++){  //ATTENZIONE filter channel

					// Calcolo del valore del pixel nella posizione corrente
					if ( (i + 1 < height) &&  (j  + 1 < width)) { //valori esterni alla matrice non vengono passati (come se valessero zero)
						sum += (input[z][i+1][j+1] ) * filter[k][z][F_DIM_MINUS1  ][F_DIM_MINUS1];
					}
					if ( (i + 1 < height) ) { //valori esterni alla matrice non vengono passati (come se valessero zero)
						sum += (input[z][i+1][j]   ) * filter[k][z][F_DIM_MINUS1 ][F_DIM_MINUS2];
					}
					if ( (i + 1 < height) && (j -  1 >= 0) ) { //valori esterni alla matrice non vengono passati (come se valessero zero)
						sum += (input[z][i+1][j-1] ) * filter[k][z][F_DIM_MINUS1 ][F_DIM_MINUS3];
					}
					if (  (j  + 1 < width)) { //valori esterni alla matrice non vengono passati (come se valessero zero)
						sum += (input[z][i]  [j+1] ) * filter[k][z][F_DIM_MINUS2 ][F_DIM_MINUS1];
					}

					sum += (input[z][i]  [j]   ) * filter[k][z][F_DIM_MINUS2 ][F_DIM_MINUS2];

					if ( (j -  1 >= 0) ) { //valori esterni alla matrice non vengono passati (come se valessero zero)
						sum += (input[z][i]  [j-1] ) * filter[k][z][F_DIM_MINUS2 ][F_DIM_MINUS3];
					}
					if ((i -  1 > -1) && (j  + 1 < width)) { //valori esterni alla matrice non vengono passati (come se valessero zero)
						sum += (input[z][i-1][j+1] ) * filter[k][z][F_DIM_MINUS3 ][F_DIM_MINUS1];
					}
					if ((i -  1 > -1) ) { //valori esterni alla matrice non vengono passati (come se valessero zero)
						sum += (input[z][i-1][j]   ) * filter[k][z][F_DIM_MINUS3][F_DIM_MINUS2];
					}
					if ((i -  1 > -1)  && (j -  1 >= 0) ) { //valori esterni alla matrice non vengono passati (come se valessero zero)
						sum += (input[z][i-1][j-1] ) * filter[k][z][F_DIM_MINUS3 ][F_DIM_MINUS3];
					}

				}
				output[k][i][j] = sum + *(bias + k + indicek); //*(bias+k)
			}
		}
	}
}


void batchAndRelu(float ***array3d, int x, int y, int z, float* mean, float* var, float* offset, float* scale, int indicek) {
	for (int k = 0; k < z; k++) {
		float currentScale = scale[k + indicek];
		float currentMean = mean[k + indicek];
		float currentVar = var[k + indicek];

		float sqrtVar = sqrt(currentVar + 1e-5);

		for (int i = 0; i < x; i++) {
			for (int j = 0; j < y; j++) {
				float normalized = (array3d[k][i][j] - currentMean) / sqrtVar;
				array3d[k][i][j] = currentScale * normalized + offset[k + indicek];
				if (array3d[k][i][j] < 0) {
					array3d[k][i][j] = 0;
				}
			}
		}
	}
}


void maxPooling(float ***input, int height, int width, int depth) {
	int i, j, k, m, n;
	float max;
	// Scorrimento della matrice di input con stride 2 (come fosse un cursore)

	for (k = 0; k < depth; k++) {
		for (i = 0, m = 0; i < height; i += 2, m++) {
			for (j = 0, n = 0; j < width; j += 2, n++) {
				max = input[k][i][j];

				// Trova il valore massimo all'interno del filtro 3x3
				for (int x = i; x < i + 3 && x < height; x++) {
					for (int y = j; y < j + 3 && y< width; y++) {
						if (input[k][x][y] > max) {
							max = input[k][x][y];
						}
					}
				}

				// Assegna il valore massimo alla matrice di output
				input[k][m][n] = max;
			}
		}
	} //viene usata la stessa matrice
	for (k = 0; k < depth; k++) {
		for (i = 0; i < height;i++) {
			for (j = 0; j < width; j++) {
				if(i>=(height+1)/2 || j>=(width+1)/2){
					input[k][i][j] = 0;
				}
			}
		}
	}
}


void maxPooling2(float ***input, int height, int width, int depth) {
	int i, j, k, m, n;
	float max;
	// Scorrimento della matrice di input con stride 2 (come fosse un cursore)

	for (k = 0; k < depth; k++) {
		for (i = 0, m = 0; i < height; i += 2, m++) {
			for (j = 0, n = 0; j < width; j += 2, n++) {
				max = input[k][i][j];

				// Trova il valore massimo all'interno del filtro 3x3
				for (int x = i+1; x > i-2 ; x--) {
					for (int y = j+1; y > j-2 ; y--) {
						if ( x < height && x >= 0 && y< width && y >= 0) {
							if(input[k][x][y] > max){
								max = input[k][x][y];
							}

						}
					}
				}

				// Assegna il valore massimo alla matrice di output
				input[k][m][n] = max;
			}
		}
	}
	for (k = 0; k < depth; k++) {
		for (i = 0; i < height;i++) {
			for (j = 0; j < width; j++) {
				if(i>=(height+1)/2 || j>=(width+1)/2){
					input[k][i][j] = 0;
				}
			}
		}
	}
}


void maxPooling3(float ***input, int height, int width, int depth) {
	int i, j, k;
	float max;
	// Scorrimento della matrice di input con stride 2 (come fosse un cursore)

	for (k = 0; k < depth; k++) {
		for (j = 0; j < width; j ++) {
			max = input[k][0][j];

			for (int x = 0; x < 13 && x < height; x++) {
				if (input[k][x][j] > max) {
					max = input[k][x][j];
				}

			}

			// Assegna il valore massimo alla matrice di output
			input[k][0][j] = max;
		}

	}
	for (k = 0; k < depth; k++) {
		for (i = 1; i < height;i++) {
			for (j = 0; j < width; j++) {
				input[k][i][j] = 0;
			}
		}
	}
}
// 13 x 7 x 48
void fclayer(float ***input, float *filter, int width, int depth, float* bias, int indicek) {
	int i, j, k;
	float sum;

	// Ciclo attraverso tutti gli elementi (i,j) della matrice output
	for(i = 0; i < 12; i++){
		sum = 0.0;
		//M2M_LOG_INFO("\r\n");
		//M2M_LOG_INFO("%d\r\n",i);
		for (k = 0; k < depth; k++) {
			for (j = 0; j < width; j++) {
				//M2M_LOG_INFO("%g",*(filter + j + (k*width) + i*(336)));
				sum += (input[k][0][j] * *(filter + j + (k*width)+ i*(336))); //*(bias + k + indicek)
			}
		}
		input[0][i+1][0] = sum + *(bias + i + indicek);
	}
}

void softmax(float ***input) {
	int i;
	float sum;

	sum = 0.0;    // Ciclo attraverso tutti gli elementi (i,j) della matrice output
	for(i = 0; i < 12; i++){
		sum += exp(input[0][i+1][0]);
	}
	for(i = 0; i < 12; i++){
		input[0][i+1][0] = exp(input[0][i+1][0])/sum ;
	}
}

void FallingTrigg_cb(UINT32 fd,  void *userdata )
{
	timer2_start();
}

void configRegister(INT32 fd, UINT8 regAddr, const char* regName, UINT8 *bytesToWrite, UINT32 byteslen, char* message)
{
	INT32 i2c_res;

	M2MB_I2C_CFG_T config;

	i2c_res = m2mb_i2c_ioctl(fd, M2MB_I2C_IOCTL_GET_CFG, (void *)&config);
	if (i2c_res != 0)
	{
		AZX_LOG_INFO("cannot get I2C channel configuration\r\n");
		return;
	}

	config.registerId = regAddr;

	i2c_res = m2mb_i2c_ioctl(fd, M2MB_I2C_IOCTL_SET_CFG, (void *)&config);
	if (i2c_res != 0)
	{
		AZX_LOG_INFO("cannot set I2C channel configuration\r\n");
		return;
	}
	AZX_LOG_INFO( "Configuring I2C Registers - Writing %d bytes into 0x%02X register (%s)...\r\n", byteslen, regAddr, regName);

	i2c_res = m2mb_i2c_write(fd, bytesToWrite, byteslen);
	if (i2c_res != byteslen)
	{
		AZX_LOG_INFO("cannot write data! error: %d\r\n", i2c_res);
		return;
	}
	else
	{
		AZX_LOG_INFO("Write: success\r\n");
	}
}

int maxim_setup(void)
{
	INT32 res;
	INT32 fd;
	UINT8 buf[256];
	CHAR dev_ID[64];

	M2MB_I2C_CFG_T config;


	/**************
	 *  Configuring the IIC device.
	 **************/
	AZX_LOG_INFO( "\nConfiguring the Maxim DVI codec\r\n" );

	//Create device name using device address in decimal base, left shifted by 1 bit
	sprintf(dev_ID, "/dev/I2C-%d", DEVICE_ADDR);
	AZX_LOG_INFO("opening channel %s\r\n", dev_ID);

	fd = m2mb_i2c_open( dev_ID, 0 );

	if (-1 == fd)
	{
		AZX_LOG_INFO("cannot open I2C channel!\r\n");
		return -1;
	}

	config.sclPin = I2C_SCL;
	config.sdaPin = I2C_SDA;

	config.registerId = 0;

	res = m2mb_i2c_ioctl(fd, M2MB_I2C_IOCTL_SET_CFG, (void *)&config);
	if (res != 0)
	{
		AZX_LOG_INFO("cannot configure I2C channel\r\n");
		return -1;
	}



	/**********
	 *  WRITING PROCEDURE
	 **********/
	configRegister( fd, INTERRUPT_ENABLE_REG, "INTERRUPT", cfg, sizeof(cfg), (char*) "config done" );
	buf[0] = 0x8A;

	configRegister( fd, SYS_SHUTDOWN_REG, "SYS_SHUTDOWN", buf,  1,  (char*) "device enabled" );
	return 0;
}

int send_process(const char* cmd, const char* cmd_str)
{
	char buf[BUFSIZE];
	FILE *fp;

	//printf("%s\r\n", cmd_str);
	if ((fp = popen(cmd, "r")) == NULL) {
		AZX_LOG_INFO("Error opening pipe!\n");
		return -1;
	}

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		// Do whatever you want here...
		//AZX_LOG_INFO("OUTPUT: %s", buf);
	}

	if(pclose(fp))  {
		AZX_LOG_INFO("Command not found or exited with error status\n");
		return -1;

	}
	return 0;
}

int startup_interface(void)
{
	if(0 != send_process("amix 'MultiMedia1 Mixer PRI_MI2S_TX' 1", "Enabling alsa mixer input on default card" ))
	{
		return -1;
	}
	if(0 != send_process("arec -D hw:0,0 -R 16000 -C 1 -T 3 /data/azc/mod/yes16kHz.wav", "register 1 second at 16kHz" ))
	{
		return -1;
	}
	return 0;
}

INT16 findVoiceStartSample( INT16 *audioData) {
	float energy;
#ifdef calcoloSoglia
	float max=0.0;
#endif
	for (int i = 0; i < audioLengthMinusOneSec - SEGMENT_SIZE; i += SEGMENT_SIZE) {
		// Calcola l'energia del segmento audio
		energy = 0.0;
		for (int j = 0; j < SEGMENT_SIZE; j++) {
			energy += (float)(audioData[i + j] * audioData[i + j]);
		}
#ifdef calcoloSoglia
		AZX_LOG_INFO("Energy: %f\r\n",energy);
		if (energy > max) {
			max = energy;
		}
	}
	AZX_LOG_INFO("Max: %f\r\n",max);
#endif
#ifndef calcoloSoglia
	// Se l'energia supera la soglia, considera il segmento come attività vocale
	if (energy > ENERGY_THRESHOLD) {
		return i;  // Restituisci il numero del campione in cui inizia l'attività vocale
	}
}
#endif
return audioLengthMinusOneSec;  // Nessun parlato rilevato
}




