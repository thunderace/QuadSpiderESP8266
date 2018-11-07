#ifndef __SERVO_H__
#define __SERVO_H__

typedef struct _servo_ {
	unsigned short pin;
	unsigned int min;
	unsigned int max;
	unsigned int home;
}SERVO;
#endif
