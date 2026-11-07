/* Serpiente (app de fábrica de senew). El tablero sale del tamaño de la PANTALLA DE
 * ESTE APARATO (se pregunta al abrir) y vive en el montón de la app; la mejor marca, en un fichero suyo.
 * El joystick (o 2/4/6/8) gira; MANTENER la dirección = turbo; OK = pausa. Un aviso del sistema encima la pausa sola. */
#include "w5.h"

W5_APP(.nombre = "Serpiente", .version = 2, .api_menor = 0, .icono_gen = W5_IC_JUEGO, .monton = 4096, .caps = W5_CAP_JOYSTICK);   /* (APX) el icono de casa. Es un JUEGO: no viene de fábrica (va en /memoria/apps) */

#define CAS 10
static int AN, AL, MAXL;
static u8 *px, *py;
static struct { u16 cabeza, largo; signed char dx, dy, ndx, ndy; u8 cx, cy, pausa, fin, turbo; u16 puntos; u32 azar, acum, version; } s;
static u16 mejor;

static u32 azar(void) { s.azar = s.azar * 1103515245u + 12345u; return s.azar >> 16; }
static int ocupa(int x, int y)
{
    unsigned i;
    for (i = 0; i < s.largo; i++) { unsigned p = (unsigned)(s.cabeza + MAXL - (int)i) % (unsigned)MAXL; if (px[p] == x && py[p] == y) return 1; }
    return 0;
}
static void comida(void) { do { s.cx = (u8)(azar() % (unsigned)AN); s.cy = (u8)(azar() % (unsigned)AL); } while (ocupa(s.cx, s.cy)); }
static void nueva(void)
{
    u32 semilla = s.azar ? s.azar : ap_ms() * 2654435761u + 1, v = s.version + 1;
    int i;
    memset(&s, 0, sizeof s);
    s.azar = semilla; s.version = v; s.largo = 4; s.cabeza = 3; s.dx = s.ndx = 1;
    for (i = 0; i < 4; i++) { px[i] = (u8)(4 + i); py[i] = (u8)(AL / 2); }
    comida();
}
static void paso(W5Ctx c)
{
    int nx, ny, dir;
    u32 cada;
    if (s.pausa || s.fin) { if (s.turbo) { s.turbo = 0; ui_sucio(c); } return; }
    dir = s.dx > 0 ? W5K_DER : s.dx < 0 ? W5K_IZQ : s.dy > 0 ? W5K_ABAJO : W5K_ARRIBA;
    s.turbo = (u8)ap_tecla_abajo(dir);
    cada = s.turbo ? 50 : (s.puntos < 30 ? 180u - s.puntos * 3u : 90);
    s.acum += 20;
    if (s.acum < cada) return;
    s.acum = 0;
    s.dx = s.ndx; s.dy = s.ndy;
    nx = px[s.cabeza] + s.dx; ny = py[s.cabeza] + s.dy;
    if (nx < 0 || ny < 0 || nx >= AN || ny >= AL || ocupa(nx, ny)) {
        s.fin = 1;
        if (s.puntos > mejor) { mejor = s.puntos; ap_fich_escribir("mejor", &mejor, sizeof mejor); }
        ui_sucio(c); return;
    }
    s.cabeza = (u16)((s.cabeza + 1) % MAXL); px[s.cabeza] = (u8)nx; py[s.cabeza] = (u8)ny;
    if (nx == s.cx && ny == s.cy) { s.puntos++; if (s.largo < MAXL - 1) s.largo++; comida(); }
    s.version++;
    ui_sucio(c);
}
static void iniciar(W5Ctx c, void *dato)
{
    static u16 m;
    (void)dato;
    if (ap_fich_leer("mejor", 0, &m, sizeof m) == (int)sizeof m && m > mejor) mejor = m;
    nueva();
    ui_cada(c, 20, paso);
}
static int tecla(W5Ctx t, int k, void *dato)
{
    int dx = 0, dy = 0;
    (void)dato;
    if (k == W5K_ARRIBA || k == W5K_2) dy = -1;
    else if (k == W5K_ABAJO || k == W5K_8) dy = 1;
    else if (k == W5K_IZQ || k == W5K_4) dx = -1;
    else if (k == W5K_DER || k == W5K_6) dx = 1;
    else if (k == W5K_OK || k == W5K_5) { if (s.fin) nueva(); else s.pausa = !s.pausa; ui_sucio(ui_mi_pantalla(t)); return 1; }
    else if (k == W5K_SEL_IZQ) { nueva(); ui_sucio(ui_mi_pantalla(t)); return 1; }
    else return ui_teclas_defecto(t, k);
    if (dx != -s.dx || dy != -s.dy) { s.ndx = (signed char)dx; s.ndy = (signed char)dy; }
    return 1;
}
static void pintar(W5Lienzo l, const W5Rect *r, void *dato)
{
    int cas = r->w / AN < r->h / AL ? r->w / AN : r->h / AL, ox, oy;
    unsigned i, barra = ui_color(W5_COL_BARRA), acento = ui_color(W5_COL_ACENTO), texto = ui_color(W5_COL_TEXTO);
    (void)dato;
    if (cas < 3) cas = 3;
    ox = r->x + (r->w - AN * cas) / 2; oy = r->y + (r->h - AL * cas) / 2;
    li_rect(l, ox, oy, AN * cas, AL * cas, barra);
    li_rect(l, ox + s.cx * cas + 2, oy + s.cy * cas + 2, cas - 4, cas - 4, acento);
    li_marco(l, ox + s.cx * cas + 1, oy + s.cy * cas + 1, cas - 2, cas - 2, acento);
    for (i = 0; i < s.largo; i++) {
        unsigned p = (unsigned)(s.cabeza + MAXL - (int)i) % (unsigned)MAXL;
        li_rect(l, ox + px[p] * cas + (i ? 1 : 0), oy + py[p] * cas + (i ? 1 : 0), cas - (i ? 2 : 0), cas - (i ? 2 : 0), i ? texto : acento);
    }
    if (s.fin || s.pausa) {
        const char *m = s.fin ? "FIN" : "PAUSA";
        int e = AN * cas >= 200 ? 4 : 2, w = li_ancho_texto(m, e, 0);
        li_velo(l, ox, oy + AL * cas / 2 - 6 * e, AN * cas, 12 * e);
        li_texto(l, ox + (AN * cas - w) / 2, oy + AL * cas / 2 - 7 * e / 2, m, W5_RGB(255, 255, 255), e, 0);
    }
}
static W5W construir(W5Ctx c, void *dato)
{
    (void)c; (void)dato;
    return Teclas(tecla, ui_andamio("Serpiente", Columna(A_CRUZADO(W5P_ESTIRAR), HIJOS(
        RellenoXY(4, 3, Fila(HIJOS(
            Texto(ui_fmt("puntos %d", s.puntos), A_COLOR(ui_color(W5_COL_ACENTO))), Hueco(),
            Texto(s.turbo && !s.fin ? "TURBO" : s.fin ? "OK: otra" : ""), Hueco(),
            Texto(ui_fmt("mejor %d", mejor), A_COLOR(ui_color(W5_COL_TENUE)))))),
        Expandido(Pintura(pintar, A_VERSION(s.version * 4 + s.fin * 2u + s.pausa))))),
        "Nueva", "Atrás"));
}
W5Pal app_suceso(int suceso, W5Pal a, W5Pal b)
{
    (void)a; (void)b;
    if (suceso == W5_ABRIR) {
        AN = ap_consulta(W5_Q_ANCHO) / CAS; AL = (ap_consulta(W5_Q_ALTO) - 50) / CAS;      /* 50 px: título, marcador y barra de teclas */
        if (AN < 8 || AL < 8 || AN > 255 || AL > 255) return 0;
        MAXL = AN * AL;
        px = ap_mem_pedir((unsigned)MAXL); py = ap_mem_pedir((unsigned)MAXL);
        if (!px || !py) return 0;
        return (W5Pal)ui_pantalla(construir, iniciar, NULL, NULL);
    }
    if (suceso == W5_PAUSA && !s.fin) s.pausa = 1;           /* un aviso del sistema encima: en pausa hasta que el jugador pulse OK */
    return 0;
}
