#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
using namespace std;


bool tabSort(const pair<vector<unsigned char>, int> & freqA, const pair<vector<unsigned char>, int> & freqB)
{
    return freqA.second > freqB.second;
}

vector<unsigned char> UTF8_Handler(ifstream & in, char & symbol)
{
    unsigned char handler; vector<unsigned char> charSet; // основная идея: хранить байты UTF-8 в наборе char-объектов
    handler = symbol;
    charSet.push_back(handler);
    if (handler < 128) return charSet; // эквивалентно проверке на соответствие шаблону 0xxxxxxx для UTF-8
    if (handler >= 192 || handler < 224) // эквивалентно проверке на соответствие шаблону 110xxxxx для UTF-8
    {
        in.get(symbol);
        handler = symbol;
        charSet.push_back(handler);
        return charSet;
    }
    if (handler >= 224 || handler < 240) // эквивалентно проверке на соответствие шаблону 1110xxxx для UTF-8
    {
        for (int i = 0; i < 2; i++)
        {
            in.get(symbol);
            handler = symbol;
            charSet.push_back(handler);
        }
        return charSet;
    }
    for (int i = 0; i < 3; i++) // если не прошла ни одна из проверок выше, то байт соответствует шаблону 11110xxx для UTF-8
    {
        in.get(symbol);
        handler = symbol;
        charSet.push_back(handler);
    }
    return charSet;
} // потом будем использовать эту функцию, чтобы аккуратно переносить UTF-8-символы в таблицу Фано при кодировании и обратно в итоговый файл при декодировании

void encoder(ifstream & in, ofstream & out)
{
    vector<pair<vector<unsigned char>, int>> frequencies; // таблица частот: ключ - символ, значение - частота символа
    bool isThere = false;
    if (in.is_open())
    {
        char symbol, symbolToUpdate; int valueToUpdate; vector<unsigned char> utf8char;
        in.get(symbol);
        utf8char = UTF8_Handler(in, symbol);
        frequencies.push_back(pair<vector<unsigned char>, int>(utf8char, 1));
        while (in.get(symbol))
        {
            utf8char = UTF8_Handler(in, symbol);
            isThere = false;
            for (int i = 0; i < frequencies.size(); i++)
            {
                if (utf8char == frequencies[i].first)
                {
                    frequencies[i].second++;
                    isThere = true;
                    break;
                }
            }
            if (!isThere)
            {
                frequencies.push_back(pair<vector<unsigned char>, int>(utf8char, 1));
            }
        } // получаем таблицу частот
        if (frequencies.empty())
        {
            cout << "Error: frequencies tab is empty. The file itself is empty or there's a bug in the code." << endl;
            exit(1);
        }
        sort(frequencies.begin(), frequencies.end(), tabSort); // сортируем по частотам в порядке убывания
        int size = frequencies.size();
        
        vector<int> initial;
        for (int i = 0; i < size; i++)
        {
            initial.push_back(i);
        }

        vector<string> codes; // хранит в себе постепенно формируемые коды
        for (int i = 0; i < size; i++)
        {
            codes.push_back("");
        }
 
        // сама формулировка метода Фано предполагает очень удобную хвостовую рекурсию,
        // но, как нам сказали на одной из лекций по алгоритмам и структурам данных,
        // компилятор умеет преобразовывать хвостовые рекурсии в обычный цикл;
        // мне стало интересно решить задачу именно через обычный цикл

        vector<vector<int>> indexSets;
        indexSets.push_back(initial);
        vector<vector<int>> newIndexSets;
        vector<int> bufferZero;
        vector<int> bufferOne;
        int onesCounter;
        int newOnesCounter;
        while (true)
        {
            onesCounter = 0;
            newOnesCounter = 0;
            for (int i = 0; i < indexSets.size(); i++)
            {
                if (indexSets[i].size() == 1)
                {
                    newIndexSets.push_back(indexSets[i]);
                    onesCounter++;
                    continue;
                }
                else
                {
                    for (int j = 0; j < indexSets[i].size()/2; j++)
                    {
                        codes[indexSets[i][j]] += '0';
                        bufferZero.push_back(indexSets[i][j]);
                    }
                    newIndexSets.push_back(bufferZero);
                    bufferZero.clear();
                    
                    for (int j = indexSets[i].size()/2; j < indexSets[i].size(); j++)
                    {
                        codes[indexSets[i][j]] += '1';
                        bufferOne.push_back(indexSets[i][j]);
                    }
                    newIndexSets.push_back(bufferOne);
                    bufferOne.clear();
                }
            }

            if (onesCounter == size) break;
            for (int i = 0; i < newIndexSets.size(); i++)
            {
                if (newIndexSets[i].size() == 1) newOnesCounter++;
            }
            if (newOnesCounter == size) break;
            else
            {
                indexSets = newIndexSets;
                newIndexSets.clear();
            }
        }

        vector<pair<vector<unsigned char>, string>> FanoTab; // составление таблицы Фано
        for (int i = 0; i < size; i++) FanoTab.push_back(pair<vector<unsigned char>, string>(frequencies[i].first, codes[i]));
        in.close();
        in.open("text.txt"); // переоткрываем файл, чтобы начать читать его с начала
        int index;
        string result = ""; // составление закодированного текста
        while (in.get(symbol)) // здесь происходит поиск в символа таблице Фано и одновременно подсчитывается длина текста, это будет нужно в дальнейшем
        {
            utf8char = UTF8_Handler(in, symbol);
            index = 0;
            while (utf8char != FanoTab[index].first) index++;
            result += FanoTab[index].second;
        }
        
        out << result.length() << ' '; // запись длины закодированного текста, чтобы потом правильно декодировать
        
        // запись таблицы Фано в файл; в первой строке пишем кол-во строк в таблице, чтобы потом верно считать её из файла с закодированным текстом
        out << size << endl;
        for (int i = 0; i < size; i++)
        {
            for (int j = 0; j < FanoTab[i].first.size(); j++) out.put(FanoTab[i].first[j]);
            out << FanoTab[i].second << endl;
        }

        // далее запись самого закодированного текста
        int encodedLength = result.length();
        char byte = 0;
        int byteDiv = 0;
        char marker = 1;
        for (int i = 0; i < encodedLength; i++) // С++ не даёт записать один бит в файл, он даёт писать туда минимум 1 байт, поэтому приходится изворачиваться
        {
            byte = byte << 1;
            if (result[i] == '1')
            {
                byte |= marker;
            }
            byteDiv++;
            if (byteDiv == 8)
            {
                out.put(byte);
                byte = 0;
                byteDiv = 0;
            }
        }
        if (byteDiv != 0)
        {
            byte = byte << (8 - byteDiv);
            out.put(byte);
        }
        in.close();
        out.close();
    }
};

void decoder(ifstream & in, ofstream & out)
{
    // Восстанавливаем данные о длине оригинального и закодированного сообщений, а также таблицу Фано 
    string data;
    string toDecode = "";
    string code;
    string strEncodedLength;
    string strFanoSize;
    getline(in, data);
    bool afterSpace = false;
    for (int i = 0; i < data.length(); i++)
    {
        if (data[i] == ' ')
        {
            afterSpace = true;
            continue;
        }
        if (!afterSpace)
        {
            strEncodedLength += data[i];
        }
        else
        {
            strFanoSize += data[i];
        }
    }
    int encodedLength = stoi(strEncodedLength);
    int size = stoi(strFanoSize);
    char symbol;
    vector<unsigned char> utf8char;
    vector<pair<vector<unsigned char>, string>> FanoTab;
    for (int i = 0; i < size; i++)
    {
        in.get(symbol);
        utf8char = UTF8_Handler(in, symbol);
        getline(in, code);
        FanoTab.push_back(pair<vector<unsigned char>, string>(utf8char, code));
    }

    unsigned char marker; unsigned char u_symbol;
    while(in.get(symbol)) // здесь читаем сам закодированный текст и переносим его в более удобный для обработки вид
    {
        u_symbol = symbol; // функция get не даёт читать напрямую в переменную типа unsigned char, хотя во многих справочных материалах указано обратное, но у меня при таком выдавало ошибку
        marker = 128;
        for (int i = 0; i < 8; i++)
        {
            if (u_symbol & marker) toDecode += '1';
            else toDecode += '0';
            marker = marker >> 1;
        }
    }

    string another_code;
    for (int i = 0; i < encodedLength; i++) // здесь уже идёт сама декодировка; так как C++ даёт читать минимум один байт из файла, в самом конце полученный из файла закодированный текст будет немного отличаться от изначального закодированного текста, поэтому мы ограничиваем цикл именно по длине, подсчитанной ранее и указанной в начале файла, чтобы исключить обработку мусора как важные данные
    {
        another_code += toDecode[i]; // в another_code пишется по одному символу из декодированного сообщения, пока не найдётся кодовое слово из таблицы Фано, равное another_code
        for (int j = 0; j < size; j++)
        {
            if (another_code == FanoTab[j].second) // после нахождения соответствующий символ пишется в файл, another_code очищается и цикл начинается заново
            {
                for (int k = 0; k < FanoTab[j].first.size(); k++) out.put(FanoTab[j].first[k]);
                another_code = "";
                break;
            }
        }
    }
    in.close();
    out.close();
}

bool mode()
{
    char input;
    while (true)
    {
        cout << "Choose working mode: encoder or decoder [e/d]: ";
        cin >> input;
        if (input == 'e') return false;
        if (input == 'd') return true;
        else cout << "Incorrect input. Please try again." << endl;
    }
}

int main()
{
    bool codeOrDecode = mode();
    if (!codeOrDecode)
    {
        ifstream einput("text.txt", ios::in);
        ofstream eoutput("resultE.bin", ios::out | ios::binary);
        if (!einput.is_open() || !eoutput.is_open())
        {
            cout << "Error: can't open file." << endl;
            exit(1);
        }
        encoder(einput, eoutput);
        cout << "Encoding done. Result is saved in resultE.bin." << endl;
    }
    else
    {
        ifstream dinput("resultE.bin", ios::binary);
        ofstream doutput("resultD.txt", ios::binary);
        if (!dinput.is_open() || !doutput.is_open())
        {
            cout << "Error: can't open file." << endl;
            exit(1);
        }
        decoder(dinput, doutput);
        cout << "Decoding done. Result is saved in resultD.txt." << endl;        
    }
}
