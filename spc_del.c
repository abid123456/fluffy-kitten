case SPC_DEL:
    i2 = len[ry] - 1;
    if (!tf -> line[ry][n]) {
        for (i = ry;  i < i2; i++)
            tf -> line[ry][i] = tf -> line[ry][i - 1];
        if (ry == eocp) {
            tf -> line[ry][len[ry] - 1] = '\0';
            break;
        }
        len[ry]--;
        i3 = tf -> width - 1;
        postrx = 1;
    } else {
        tf -> line[ry][n] = 0;
        i3 = rx;
        postrx = tf -> width - rx;
        do ry++;
        while (ry < maxy && !tf -> line[ry][n]);
    }
