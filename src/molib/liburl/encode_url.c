#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <inttypes.h>

#include "url_interface.h"

/* Replace escape sequences in an URL (or a part of an URL) */
/* works like strcpy(), but without return argument,
   except that outbuf == inbuf is allowed */
void url_unescape_string(char *outbuf, const char *inbuf)
{
	unsigned char c, c1, c2;
	int i, len = strlen(inbuf);
	for (i = 0; i < len; i++) {
		c = inbuf[i];
		if (c == '%' && i < len - 2) {
			c1 = toupper(inbuf[i+1]);
			c2 = toupper(inbuf[i+2]);
			if (((c1 >= '0' && c1 <= '9') || (c1 >= 'A' && c1 <= 'F')) &&
				((c2 >= '0' && c2 <= '9') || (c2 >= 'A' && c2 <= 'F'))) {
				if (c1 >= '0' && c1 <= '9')
					c1 -= '0';
				else
					c1 -= 'A'-10;
				if (c2 >= '0' && c2 <= '9')
					c2 -= '0';
				else
					c2 -= 'A' - 10;
				c = (c1<<4) + c2;
				i = i + 2; /*only skip next 2 chars if valid esc*/
			}
		}
		*outbuf++ = c;
	}
	*outbuf++ = '\0'; /*add nullterm to string*/
}

static void url_escape_string_part(char *outbuf, const char *inbuf)
{
	unsigned char c, c1, c2;
	int i, len = strlen(inbuf);

	for  (i = 0; i < len; i++) {
		c = inbuf[i];
		if ((c >= 'A' && c <= 'Z') ||
				(c >= 'a' && c <= 'z') ||
				(c >= '0' && c <= '9')) {
			*outbuf++ = c;
		} else {
			/* all others will be escaped */
			c1 = ((c & 0xf0) >> 4);
			c2 = (c & 0x0f);
			if (c1 < 10)
				c1 += '0';
			else
				c1 += 'A' - 10;
			if (c2 < 10)
				c2 += '0';
			else
				c2 += 'A' - 10;
			*outbuf++ = '%';
			*outbuf++ = c1;
			*outbuf++ = c2;
		}
	}
	*outbuf++ = '\0';
}

/* Replace specific characters in the URL string by an escape sequence */
/* works like strcpy(), but without return argument */
void url_escape_string(char *outbuf, const char *inbuf)
{
	unsigned char c;
	int i = 0, j, len = strlen(inbuf);
	char *tmp, *unesc = NULL, *in;

	/* Look if we have an ip6 address, if so skip it there is
	no need to escape anything in there.*/
	tmp = strstr(inbuf, "://[");
	if (tmp) {
		tmp = strchr(tmp+4, ']');
		if (tmp && (tmp[1] == '/' || tmp[1] == ':' ||
					tmp[1] == '\0')) {
			i = tmp + 1 - inbuf;
			strncpy(outbuf, inbuf, i);
			outbuf += i;
			tmp = NULL;
		}
	}

	tmp = NULL;
	while (i < len) {
		/* look for the next char that must be kept */
		for (j = i; j < len; j++) {
			c = inbuf[j];
			if (c == '-' || c == '_' || c == '.' ||
					c == '!' || c == '~' ||	/* mark characters */
					c == '*' || c == '\'' ||
					c == '(' || c == ')' || /* do not touch escape character */
					c == ';' || c == '/' || c == '?' ||
					c == ':' || c == '@' || /* reserved characters */
					c == '&' || c == '=' || c == '+' ||
					c == '$' || c == ',') /* see RFC 2396 */
				break;
		}
		/* we are on a reserved char, write it out */
		if (j == i) {
			*outbuf++ = c;
			i++;
			continue;
		}
		/* we found one, take that part of the string */
		if (j < len) {
			if (!tmp)
				tmp = malloc(len + 1);
			strncpy(tmp, inbuf + i, j - i);
			tmp[j-i] = '\0';
			in = tmp;
		} else /* take the rest of the string */
			in = (char *)inbuf + i;

		if (!unesc)
			unesc = malloc(len + 1);
		/* unescape first to avoid escaping escape */
		url_unescape_string(unesc, in);
		/* then escape, including mark and other reserved chars
		 that can come from escape sequences */
		url_escape_string_part(outbuf, unesc);
		outbuf += strlen(outbuf);
		i += strlen(in);
	}
	*outbuf = '\0';
	free(tmp);
	free(unesc);
}


