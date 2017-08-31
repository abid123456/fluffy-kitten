case SPC_DEL:
    if (!tf -> line[ry][n]) {
        i2 = len[ry] - 1;
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
    /* shifting */
    for (i2 = ry + 1; i2 < eocp; i2++) {
        for (i = i3; i < tf -> width; i++)
            tf -> line[i2 - 1][i] = tf -> line[i2][i - i3];
        for (i = 0; i < i3; i++)
            tf -> line[i2][i] = tf -> line[i2][postrx + i];
    }
    if (len[eocp] <= postrx) {
        i2 = i3 + len[eocp--];
        for (i = i3; i < i2; i++) {
            tf -> line[eocp][i] = tf -> line[eocp + 1][i - i3];
            tf -> line[eocp + 1][i - i3] = '\0';
        }
        while (i < tf -> width)
            tf -> line[eocp][i++] = '\0';
        len[eocp] = len[ry] + len[eocp + 1];
        len[ry] = tf -> width;
        len[eocp + 1] = 0;
        shift_up(tf, eocp + 2, maxy--, len);
    } else {
        for (i = i3; i < tf -> width; i++)
            tf -> line[eocp - 1][i] = tf -> line[eocp][i - i3];
        i2 = len[eocp] - postrx;
        for (i = 0; i < i2; i++)
            tf -> line[eocp][i] = tf -> line[eocp][postrx + i];
        while (i < len[eocp])
            tf -> line[eocp][i++] = '\0';
        len[ry] = tf -> width;
        len[eocp] -= postrx;
    }
    break;
