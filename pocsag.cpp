#define uchar unsigned char
#define uint unsigned int
String comdata = "";
unsigned long addr;

// 发送格式 a+1234567+b+信息（a=N或P 向位，1234567 呼机地址码, b=1-4 铃声）
// 如：N12345671测试！

int PTT = 10; // PTT控制端
int TX = 9;   // 数据输出端
int ys = 819; // 延时
unsigned long tem;
uchar Tx_Num;

unsigned long calc_bch_and_parity(unsigned long cw_e) // BCH校验和奇偶校验函数
{
  uchar i;
  uchar parity = 0;       // 奇偶校验计数
  unsigned long local_cw; // 临时存放数
  local_cw = cw_e;        // 保存cw_e参数值
  for (i = 1; i <= 21; i++, cw_e <<= 1)
    if (cw_e & 0x80000000)
      cw_e ^= 0xED200000;
  cw_e = cw_e & 0xFFC00000; // 保留前10位，BCH校验值共11位，只保留前10位有效数据
  local_cw |= (cw_e >> 21); // BCH校验数移至第22位到31位，BCH共10位，并和原始数据相加
  cw_e = local_cw;
  for (i = 0; i < 31; i++, cw_e <<= 1)
    if (cw_e & 0x80000000)
      parity++;
  if (parity % 2)
    local_cw += 1; // 从1至31位判断为奇数则后面加1补充为偶数
  return local_cw;
}

unsigned long calc_addr(unsigned long add, uchar fun) // 地址转换，第1参数为地址，第2参数为功能
{
  unsigned long adr;
  unsigned long tem;
  Tx_Num = (uchar)(add & 0x00000007); // 获取地址发射的帧位次，111位第7帧，后3位地址数据隐藏不发送，接收按帧位还原
  adr = 0x00;
  adr = add & 0xFFFFFFF8; // 去掉地址码后3位
  adr = adr << 10;        // 地址左移10位
  tem = 0x00;
  tem = fun;       // 功能位
  tem = tem << 11; // 功能位左移11位，功能位为00 01 10 11四种状态，代表4个地址码
  adr = adr | tem; // 地址码和功能位合成地址码；
  return adr;
}

void Send_start(unsigned long s)
{
  uchar i, n;
  unsigned long tem;
  for (i = 0; i < 20; i++) // 发送576个前导10101010101010
  {
    tem = s;
    for (n = 0; n < 32; n++)
    {
      if (tem & 0x80000000)
      {
        digitalWrite(TX, HIGH);
      }
      else
      {
        digitalWrite(TX, LOW);
      }
      delayMicroseconds(ys); // 等待延时结束 0.833ms
      tem <<= 1;
    }
  }
}

void Send_nill() // 发送闲置位
{
  uchar n;
  unsigned long s = 0x7A89C197;
  for (n = 0; n < 32; n++)
  {
    if (s & 0x80000000)
    {
      digitalWrite(TX, LOW);
    }
    else
    {
      digitalWrite(TX, HIGH);
    }
    delayMicroseconds(ys); // 等待延时结束 0.833ms
    s <<= 1;
  }
}

void Send_Num(unsigned long s, char npi) // 发送数据
{
  uchar n;
  int xx0, xx1;
  if (npi == 'P')
  {
    xx1 = 1;
    xx0 = 0;
  }
  if (npi == 'N')
  {
    xx1 = 0;
    xx0 = 1;
  }
  for (n = 0; n < 32; n++)
  {
    if (s & 0x80000000)
    {
      digitalWrite(TX, xx1);
      // Serial.print(xx1);
    }
    else
    {
      digitalWrite(TX, xx0);
      // Serial.print(xx0);
    }
    delayMicroseconds(ys); // 等待延时结束 0.833ms
    s <<= 1;
  }
}

void setup()
{
  Serial.begin(9600); // 设置波特率为9600，一般是这个波特率
  pinMode(TX, OUTPUT);
  pinMode(PTT, OUTPUT);
}

void loop()
{
  while (Serial.available() > 0) // 读取串口数据
  {
    comdata += char(Serial.read());
    delay(2);
  }

  if (comdata.length() > 0) // 如果有数据进入处理
  {
    String np = comdata.substring(0, 1);
    char npzf = toupper(np[0]);
    String ly = comdata.substring(8, 9);
    int lyi = ly[0];
    String dz = comdata.substring(1, 8);        // 获取地址码
    unsigned long az = dz.toInt();              // 地址码s转类型
    tem = calc_addr(az, lyi - 48);              // 地址码移位处理
    addr = calc_bch_and_parity(tem);            // 地址码BCH校验
    String msgdata = comdata.substring(9, 200); // 截取信息码内容
    int len = msgdata.length() + 10;            // 获取信息码字节长度(+4为了让长度大于1帧，就不用1个汉字还用做判断了)

    byte hui[200] = {0};

    digitalWrite(PTT, LOW);        // 对讲机ppt
    delay(500);                    // 延时500ms
    unsigned long mess[200] = {0}; // 信息码数组
                                   // 信息码数组元素
    unsigned long msg;

    int ll = 0;
    int wz = 0;

    if (!(msgdata[0] & 0x80))
    {
      hui[wz] = 0x0f;
      wz++;
    }

    for (int i = 0; i < len; i++)
    {
      if (!(msgdata[i] & 0x80) && msgdata[i - 1] & 0x80 && i > 0)
      {
        hui[wz] = 0x0f;
        wz++;
      }
      hui[wz] = msgdata[i];
      wz++;
      if (!(msgdata[i] & 0x80) && msgdata[i + 1] & 0x80)
      {
        hui[wz] = 0x0e;
        wz++;
      }
    }

    wz = 0;

    int k = 1;
    for (int n = 0; n <= len; n++)
    {
      for (int j = 0; j < 7; j++)
      {
        if (hui[wz] & 0x01)
        {
          msg |= 0x00000001;
        }
        msg <<= 1;
        hui[wz] >>= 1;
        ll++;
        if (ll % 20 == 0)
        {
          msg <<= 10;
          msg |= 0x80000000;
          mess[k] = calc_bch_and_parity(msg);
          msg = 0;
          k++;
        }
      }
      wz++;
    }

    Send_start(0xAAAAAAAA);       // 发送前言码
    Send_Num(0x7CD215D8, npzf);   // 发送同步码
    mess[0] = addr;               // 地址码放入信息码第一码字
    for (int j = 0; j < 120; j++) // 循环发送信息码
    {
      if (mess[j] != 0)
      {
        Send_Num(mess[j], npzf);
      } // 数组元素不为0的发送
      Serial.println(mess[j], BIN);

      if ((j + 1) % 16 == 0 && j != 0)
      {
        Send_Num(0x7CD215D8, npzf);
      } // 每隔8帧发送一个同步码
    }
    Send_Num(0x7A89C197, npzf); // 末尾发送闲置码
    comdata = "";
    msgdata = "";
  }

  digitalWrite(PTT, HIGH);
}
