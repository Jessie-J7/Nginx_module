///
/// @file    aes.c
/// @author  faith(kh_faith@qq.com)
/// @date    2018-06-29 11:19:47
///

#include "func.h"
/**
 *  *  * S盒
 *   *   */
static const size_t S[16][16] = { {0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76},
	{0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0},
	{0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15},
	{0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75},
	{0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84},
	{0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf},
	{0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8},
	{0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2},
	{0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73},
	{0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb},
	{0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79},
	{0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08},
	{0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a},
	{0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e},
	{0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf},
	{0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16} };

/**
 *  *  * 逆S盒
 *   *   */
static const size_t S2[16][16] = { {0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb},
	{0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb},
	{0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e},
	{0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25},
	{0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92},
	{0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84},
	{0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06},
	{0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b},
	{0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73},
	{0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e},
	{0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b},
	{0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4},
	{0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f},
	{0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef},
	{0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61},
	{0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d} };

static ngx_int_t getLeft4Bit(ngx_int_t num) {
	ngx_int_t left = num & 0x000000f0;
	return left >> 4;
}

static ngx_int_t getRight4Bit(ngx_int_t num) {
	return num & 0x0000000f;
}

static size_t getNumFromSBox(ngx_int_t index) {
	ngx_int_t row = getLeft4Bit(index);
	ngx_int_t col = getRight4Bit(index);
	return S[row][col];
}

static ngx_int_t getIntFromChar(u_char c) {
	ngx_int_t result = (ngx_int_t) c;
	return result & 0x000000ff;
}

static void convertToIntArray(u_char *str,ngx_int_t pa[4][4]){
	ngx_int_t k=0;
	for (ngx_int_t i=0;i<4;i++){
		for(ngx_int_t j=0;j<4;j++){
			pa[j][i] = getIntFromChar(str[k]);
			k++;
		}
	}
}

//static void printArray(size_t a[4][4]) {
//	for(ngx_int_t i=0;i<4;i++){
//		for(ngx_int_t j=0;j<4;j++){
//			ngx_log_stderr(0,"a[%d][%d] = 0x%Xd ",i,j,a[i][j]);
//		}
//		ngx_log_stderr(0,"\n");
//	}
//	ngx_log_stderr(0,"\n");
//}

//static void printASSCI(u_char *str, ngx_int_t len){
//	ngx_log_stderr(0,"printASSCI()");
//	for(ngx_int_t i=0; i<len; i++)
//		ngx_log_stderr(0,"0x%Xd ",getIntFromChar(str[i]));
//}

static ngx_int_t getWordFromStr(u_char *str) {
	ngx_int_t one = getIntFromChar(str[0]);
	one = one << 24;
	ngx_int_t two = getIntFromChar(str[1]);
	two = two << 16;
	ngx_int_t three = getIntFromChar(str[2]);
	three = three << 8;
	ngx_int_t four = getIntFromChar(str[3]);
	return one | two | three | four;
}

static void splitIntToArray(ngx_int_t num,ngx_int_t array[4]){
	ngx_int_t one = num >> 24;
	array[0] = one & 0x000000ff;
	ngx_int_t two = num >> 16;
	array[1] = two & 0x000000ff;
	ngx_int_t three = num >> 8;
	array[2] = three & 0x000000ff;
	array[3] = num & 0x000000ff;
}

static void leftLoop4int(ngx_int_t array[4],ngx_int_t step) {
	ngx_int_t temp[4];
	for(ngx_int_t i=0;i<4;i++)
		temp[i] = array[i];
	ngx_int_t index = step % 4 == 0 ? 0 :step % 4;
	for(ngx_int_t i=0;i<4;i++)
	{
		array[i] = temp[index];
		index++;
		index = index % 4;
	}
}

static ngx_int_t mergeArrayToInt(ngx_int_t array[4]) {
	ngx_int_t one = array[0] << 24;
	ngx_int_t two = array[1] << 16;
	ngx_int_t three = array[2] << 8;
	ngx_int_t four = array[3];
	return one | two | three | four;
}

static const ngx_int_t Rcon[10] = { 0x01000000, 0x02000000,
	0x04000000, 0x08000000,
	0x10000000, 0x20000000,
	0x40000000, 0x80000000,
	0x1b000000, 0x36000000 };

static ngx_int_t T(ngx_int_t num,ngx_int_t round){
	ngx_int_t numArray[4];
	splitIntToArray(num,numArray);
	leftLoop4int(numArray,1);

	for(int i = 0;i < 4;i++)
		numArray[i] = getNumFromSBox(numArray[i]);

	ngx_int_t result = mergeArrayToInt(numArray);
	return result ^ Rcon[round];
}

static ngx_int_t w[60];

static void extendKey(u_char *key) {
	ngx_log_stderr(0,"extendKey()");
	for(ngx_int_t i = 0;i < 4;i++)
		w[i] = getWordFromStr(key + i * 4);
	for(ngx_int_t i = 4,j = 0;i < 4;i++){
		if(i % 4 == 0){
			w[i] = w[i - 4] ^ T(w[i - 1],j);
			j++;
		}else{
			w[i] = w[i - 4] ^ w[i - 1];
		}
	}
}

static void addRoundKey(ngx_int_t array[4][4],ngx_int_t round){
	ngx_int_t warray[4];
	for(ngx_int_t i=0;i<4;i++)
	{
		splitIntToArray(w[round * 4 + i],warray);
		for(ngx_int_t j = 0;j < 4;j++) {
			array[j][i] = array[j][i] ^ warray[j];
		}
	}
}

static void subBytes(ngx_int_t array[4][4]) {
	for(ngx_int_t i=0;i<4;i++)
		for(ngx_int_t j=0;j<4;j++)
			array[i][j] = getNumFromSBox(array[i][j]);
}

/**
 *  * 行移位
 *   */
static void shiftRows(ngx_int_t array[4][4]) {
	ngx_int_t rowTwo[4], rowThree[4], rowFour[4];
	//复制状态矩阵的第2,3,4行
	for(ngx_int_t i = 0; i < 4; i++) {
		rowTwo[i] = array[1][i];
		rowThree[i] = array[2][i];
		rowFour[i] = array[3][i];
	}
	//循环左移相应的位数
	leftLoop4int(rowTwo, 1);
	leftLoop4int(rowThree, 2);
	leftLoop4int(rowFour, 3);

	//把左移后的行复制回状态矩阵中
	for(ngx_int_t i = 0; i < 4; i++) {
		array[1][i] = rowTwo[i];
		array[2][i] = rowThree[i];
		array[3][i] = rowFour[i];
	}
}

/**
 * 列混合要用到的矩阵
 */
static const ngx_int_t colM[4][4] = { {2, 3, 1, 1},
	{1, 2, 3, 1},
	{1, 1, 2, 3},
	{3, 1, 1, 2} };

static ngx_int_t GFMul2(ngx_int_t s) {
	ngx_int_t result = s << 1;
	ngx_int_t a7 = result & 0x00000100;

	if(a7 != 0) {
		result = result & 0x000000ff;
		result = result ^ 0x1b;
	}

	return result;
}

static ngx_int_t GFMul3(ngx_int_t s) {
	return GFMul2(s) ^ s;
}

static ngx_int_t GFMul4(ngx_int_t s) {
	return GFMul2(GFMul2(s));
}

static ngx_int_t GFMul8(ngx_int_t s) {
	return GFMul2(GFMul4(s));
}

static ngx_int_t GFMul9(ngx_int_t s) {
	return GFMul8(s) ^ s;
}

static ngx_int_t GFMul11(ngx_int_t s) {
	return GFMul9(s) ^ GFMul2(s);
}

static ngx_int_t GFMul12(ngx_int_t s) {
	return GFMul8(s) ^ GFMul4(s);
}

static ngx_int_t GFMul13(ngx_int_t s) {
	return GFMul12(s) ^ s;
}

static ngx_int_t GFMul14(ngx_int_t s) {
	return GFMul12(s) ^ GFMul2(s);
}

/**
 * GF上的二元运算
 */
static ngx_int_t GFMul(ngx_int_t n, ngx_int_t s) {
	ngx_int_t result=-1;

	if(n == 1)
		result = s;
	else if(n == 2)
		result = GFMul2(s);
	else if(n == 3)
		result = GFMul3(s);
	else if(n == 0x9)
		result = GFMul9(s);
	else if(n == 0xb)//11
		result = GFMul11(s);
	else if(n == 0xd)//13
		result = GFMul13(s);
	else if(n == 0xe)//14
		result = GFMul14(s);

	return result;
}
/**
 * 列混合
 */
static void mixColumns(ngx_int_t array[4][4]) {

	ngx_int_t tempArray[4][4];

	for(ngx_int_t i = 0; i < 4; i++)
		for(ngx_int_t j = 0; j < 4; j++)
			tempArray[i][j] = array[i][j];

	for(ngx_int_t i = 0; i < 4; i++)
		for(ngx_int_t j = 0; j < 4; j++){
			array[i][j] = GFMul(colM[i][0],tempArray[0][j]) ^ GFMul(colM[i][1],tempArray[1][j]) 
				^ GFMul(colM[i][2],tempArray[2][j]) ^ GFMul(colM[i][3], tempArray[3][j]);
		}
}
/**
 * 把4X4数组转回字符串
 */
static void convertArrayToStr(ngx_int_t array[4][4], u_char *str) {
	ngx_log_stderr(0,"convertArrayToStr()");
	for(ngx_int_t i = 0; i < 4; i++)
		for(ngx_int_t j = 0; j < 4; j++)
			*str++ = (u_char)array[j][i]; 
}
/**
 * 检查密钥长度
 */
static ngx_int_t checkKeyLen(ngx_int_t len) {
	ngx_log_stderr(0,"checkKeyLen()");
	if(len == 32)
		return 1;
	else
		return 0;
}

/**
 * 参数 p: 明文的字符串数组。
 * 参数 plen: 明文的长度。
 * 参数 key: 密钥的字符串数组。
 */
ngx_int_t aes(u_char *p, ngx_int_t plen, u_char *key){
	ngx_log_stderr(0,"aes()");
	ngx_int_t keylen = ngx_strlen(key);
	if(!checkKeyLen(keylen)) {
		ngx_log_stderr(0,"密钥字符长度错误！长度必须为16、24和32。当前长度为%d\n",keylen);
		return -1;
	}

	extendKey(key);//扩展密钥
	ngx_int_t pArray[4][4];

	for(ngx_int_t k = 0; k < plen; k += 16) { 
		convertToIntArray(p + k, pArray);

		addRoundKey(pArray, 0);//一开始的轮密钥加

		for(ngx_int_t i = 1; i < 14; i++){//前13轮

			subBytes(pArray);//字节代换

			shiftRows(pArray);//行移位

			mixColumns(pArray);//列混合

			addRoundKey(pArray, i);

		}

		//第14轮
		subBytes(pArray);//字节代换

		shiftRows(pArray);//行移位

		addRoundKey(pArray, 14);

		convertArrayToStr(pArray, p + k);
	}
	return 0;
}

/**
 *  * 根据索引从逆S盒中获取值
 *   */
static ngx_int_t getNumFromS1Box(ngx_int_t index) {
	ngx_int_t row = getLeft4Bit(index);
	ngx_int_t col = getRight4Bit(index);
	return S2[row][col];
}
/**
 *  * 逆字节变换
 *   */
static void deSubBytes(ngx_int_t array[4][4]) {
	for(ngx_int_t i = 0; i < 4; i++)
		for(ngx_int_t j = 0; j < 4; j++)
			array[i][j] = getNumFromS1Box(array[i][j]);
}
/**
 *  * 把4个元素的数组循环右移step位
 *   */
static void rightLoop4ngx_int_t(ngx_int_t array[4], ngx_int_t step) {
	ngx_int_t temp[4];
	for(ngx_int_t i = 0; i < 4; i++)
		temp[i] = array[i];

	ngx_int_t index = step % 4 == 0 ? 0 : step % 4;
	index = 3 - index;
	for(ngx_int_t i = 3; i >= 0; i--) {
		array[i] = temp[index];
		index--;
		index = index == -1 ? 3 : index;
	}
}

/**
 *  * 逆行移位
 *   */
static void deShiftRows(ngx_int_t array[4][4]) {
	ngx_int_t rowTwo[4], rowThree[4], rowFour[4];
	for(ngx_int_t i = 0; i < 4; i++) {
		rowTwo[i] = array[1][i];
		rowThree[i] = array[2][i];
		rowFour[i] = array[3][i];
	}

	rightLoop4ngx_int_t(rowTwo, 1);
	rightLoop4ngx_int_t(rowThree, 2);
	rightLoop4ngx_int_t(rowFour, 3);

	for(ngx_int_t i = 0; i < 4; i++) {
		array[1][i] = rowTwo[i];
		array[2][i] = rowThree[i];
		array[3][i] = rowFour[i];
	}
}
/**
 *  * 逆列混合用到的矩阵
 *   */
static const ngx_int_t deColM[4][4] = { {0xe, 0xb, 0xd, 0x9},
	{0x9, 0xe, 0xb, 0xd},
	{0xd, 0x9, 0xe, 0xb},
	{0xb, 0xd, 0x9, 0xe} };

/**
 *  * 逆列混合
 *   */
static void deMixColumns(ngx_int_t array[4][4]) {
	ngx_int_t tempArray[4][4];

	for(ngx_int_t i = 0; i < 4; i++)
		for(ngx_int_t j = 0; j < 4; j++)
			tempArray[i][j] = array[i][j];

	for(ngx_int_t i = 0; i < 4; i++)
		for(ngx_int_t j = 0; j < 4; j++){
			array[i][j] = GFMul(deColM[i][0],tempArray[0][j]) ^ GFMul(deColM[i][1],tempArray[1][j]) 
				^ GFMul(deColM[i][2],tempArray[2][j]) ^ GFMul(deColM[i][3], tempArray[3][j]);
		}
}
/**
 *  * 把两个4X4数组进行异或
 *   */
static void addRoundTowArray(ngx_int_t aArray[4][4],ngx_int_t bArray[4][4]) {
	for(ngx_int_t i = 0; i < 4; i++)
		for(ngx_int_t j = 0; j < 4; j++)
			aArray[i][j] = aArray[i][j] ^ bArray[i][j];
}
/**
 *  * 从4个32位的密钥字中获得4X4数组，
 *   * 用于进行逆列混合
 *    */
static void getArrayFrom4W(ngx_int_t i, ngx_int_t array[4][4]) {
	ngx_int_t index = i * 4;
	ngx_int_t colOne[4], colTwo[4], colThree[4], colFour[4];
	splitIntToArray(w[index], colOne);
	splitIntToArray(w[index + 1], colTwo);
	splitIntToArray(w[index + 2], colThree);
	splitIntToArray(w[index + 3], colFour);

	for(ngx_int_t i = 0; i < 4; i++) {
		array[i][0] = colOne[i];
		array[i][1] = colTwo[i];
		array[i][2] = colThree[i];
		array[i][3] = colFour[i];
	}

}

/**
 *  * 参数 c: 密文的字符串数组。
 *   * 参数 clen: 密文的长度。
 *    * 参数 key: 密钥的字符串数组。
 *     */
ngx_int_t deAes(u_char *c, ngx_int_t clen, u_char *key) {

	ngx_int_t keylen = ngx_strlen(key);
	if(clen == 0 || clen % 16 != 0) {
		ngx_log_stderr(0,"密文字符长度必须为16的倍数！现在的长度为%d",clen);
		return -1;
	}

	if(!checkKeyLen(keylen)) {
		ngx_log_stderr(0,"密钥字符长度错误！长度必须为16、24和32。当前长度为%d",keylen);
		return -1;
	}

	extendKey(key);//扩展密钥
	ngx_int_t cArray[4][4];
	for(ngx_int_t k = 0; k < clen; k += 16) {
		convertToIntArray(c + k, cArray);


		addRoundKey(cArray, 14);

		ngx_int_t wArray[4][4];
		for(ngx_int_t i = 13; i >= 1; i--) {
			deSubBytes(cArray);

			deShiftRows(cArray);

			deMixColumns(cArray);
			getArrayFrom4W(i, wArray);
			deMixColumns(wArray);

			addRoundTowArray(cArray, wArray);
		}

		deSubBytes(cArray);

		deShiftRows(cArray);

		addRoundKey(cArray, 0);

		convertArrayToStr(cArray, c + k);

	}
	return 0;
}
