#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>
#include <xosd.h>

#define DISPLAY_FONT   "-*-helvetica-*-r-*-*-24-*-*-*-*-*-*-*"
#define DISPLAY_OFFSET 4

int display(int val1, int val2, char op, int res, int delay)
{
	xosd *osd;
	int ret = 0;

	osd = xosd_create(2);
	if (!osd)
		return -2;

	setlocale(LC_ALL, "");

	if ((ret = xosd_set_shadow_offset(osd, 4)) != 0)
		goto out;
	if ((ret = xosd_set_colour(osd, "lawngreen")) != 0)
		goto out;
	if ((ret = xosd_set_outline_colour(osd, "black")) != 0)
		goto out;
	if ((ret = xosd_set_font(osd, DISPLAY_FONT)) != 0)
		goto out;
	if ((ret = xosd_set_pos(osd, XOSD_middle)) != 0)
		goto out;
	if ((ret = xosd_set_align(osd, XOSD_center)) != 0)
		goto out;

	/* xosd_display diverges the behavior from the manual, it is
	 * returning always -1, instead of 0 refered in docs. */
	xosd_display(osd, 0, XOSD_printf, "%d %c %d ???", val1, op, val2);
	sleep(3);
	xosd_hide(osd);
	sleep(delay);

	if ((ret = xosd_set_colour(osd, "red")) != 0)
		goto out;
	xosd_display(osd, 0, XOSD_printf,
		     "%d %c %d = %d !!!", val1, op, val2, res);
	sleep(4);

out:
	xosd_destroy(osd);
	return ret;
}


void calc(int *val1, int *val2, char *op, int *res, int *delay)
{
	char ops[] = {'+', '-', '*', '/'};

	*delay = 6;

	srandom((unsigned) time(0));

	do {
		*op = ops[random() % (sizeof(ops)/sizeof(ops[0]))];
		*val1 = random() % 100;
		*val2 = random() % 100;

		switch (*op) {
		case '+':
			*res = *val1 + *val2;
			break;
		case '-':
			*res = *val1 - *val2;
			break;
		case '*':
			*res = *val1 * *val2;
			if (*val2 > 12)
				*delay += 6;
			break;
		case '/':
			if (*val2 == 0 || *val2 > 10)
				continue;
			*res = *val1 / *val2;
		}
	} while (0);
}


int main(int argc, char **argv)
{
	
	int val1, val2, res, delay;
	char op;
	int ret;

	calc(&val1, &val2, &op, &res, &delay);

	if((ret = display(val1, val2, op, res, delay)) != 0) {
		perror("failed");
		return -ret;
	}

	return 0;
}
