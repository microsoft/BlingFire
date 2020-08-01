#include "FAConfig.h"

extern "C" const int TextToSentences(const char * pInUtf8Str, int InUtf8StrByteCount, char * pOutUtf8Str, const int MaxOutUtf8StrByteCount);
extern "C" const int TextToWords(const char * pInUtf8Str, int InUtf8StrByteCount, char * pOutUtf8Str, const int MaxOutUtf8StrByteCount);


extern "C"
int main()
{
    printf("Quick test for blingfiretokdll.cpp\n");

    const int MaxOutSize = 2048;
    char Out[MaxOutSize];

    const char * pIn = "Hello Wêreld! Përshendetje Botë! ሰላም ልዑል! مرحبا بالعالم! Բարեւ աշխարհ! Kaixo Mundua! Прывітанне Сусвет! ওহে বিশ্ব! Здравей свят! Hola món! Moni Dziko Lapansi! 你好世界！ Pozdrav svijete! Ahoj světe! Hej Verden! Hallo Wereld! Hello World! Tere maailm! Hei maailma! Bonjour monde! Hallo wrâld! გამარჯობა მსოფლიო! Hallo Welt! Γειά σου Κόσμε! Sannu Duniya! שלום עולם! नमस्ते दुनिया! Helló Világ! Halló heimur! Ndewo Ụwa! Halo Dunia! Ciao mondo! こんにちは世界！ Сәлем Әлем! សួស្តី​ពិភពលោក! Салам дүйнө! ສະ​ບາຍ​ດີ​ຊາວ​ໂລກ! Sveika pasaule! Labas pasauli! Moien Welt! Здраво свету! Hai dunia! ഹലോ വേൾഡ്! Сайн уу дэлхий! မင်္ဂလာပါကမ္ဘာလောက! नमस्कार संसार! Hei Verden! سلام نړی! سلام دنیا! Witaj świecie! Olá Mundo! ਸਤਿ ਸ੍ਰੀ ਅਕਾਲ ਦੁਨਿਆ! Salut Lume! Привет мир! Hàlo a Shaoghail! Здраво Свете! Lefatše Lumela! හෙලෝ වර්ල්ඩ්! Pozdravljen svet! ¡Hola Mundo! Halo Dunya! Salamu Dunia! Hej världen! Салом Ҷаҳон! สวัสดีชาวโลก! Selam Dünya! Привіт Світ! Salom Dunyo! Chào thế giới! Helo Byd! Molo Lizwe! העלא וועלט! Mo ki O Ile Aiye! Sawubona Mhlaba!";

    int ActualSize = TextToSentences(pIn, strlen(pIn), Out, MaxOutSize);
    if(0 < ActualSize && ActualSize < MaxOutSize)
    {
        printf("Ouput Sentences:\n");
        printf("%s", Out);
        printf("\n");
    }
    else 
    {
        printf("ERROR calling TextToSentences.\n");
    }
    printf("\n");

    ActualSize = TextToWords(pIn, strlen(pIn), Out, MaxOutSize);
    if(0 < ActualSize && ActualSize < MaxOutSize)
    {
        printf("Ouput Words:\n");
        printf("%s", Out);
        printf("\n");
    }
    else 
    {
        printf("ERROR calling TextToWords.\n");
    }
    printf("\n");
}
