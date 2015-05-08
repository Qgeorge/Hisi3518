
#include "ipc_hk.h"

/*
char* addr2string(unsigned addr)
{
	byte * b = (byte *)&addr;
	char buffer[32];
	sprintf(buffer, "%d.%d.%d.%d", b[3], b[2], b[1], b[0]);
	return buffer;
}

void reboot(unsigned short subtype)
{
	//log1(time(0), c2d::log1_reboot, subtype);
	system("sync");
	usleep(1000000); // 1√Î
	system("reboot\n");
}

char* GetTime(time_t timer)
{
	tm *p = localtime(&timer);
	char buf[128];
	sprintf(buf,"%04d%02d%02d-%02d%02d%02d",p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
	return buf;
}

#define VM_TIME_MHZ 1000000

long long time_get_tick()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * (long long)VM_TIME_MHZ + (long long)tv.tv_usec;
}

long long time_get_frequency()
{
    return (long long)VM_TIME_MHZ;
}

void execute(const char * command){
	if(0 == vfork()) {
		execl("/bin/sh", "sh", "-c", command, (char *) 0);
		_exit(127);
	}
}

int system_time_kill(const char *command, int s)
{
	int wait_val, pid, ret;
	__sighandler_t save_quit, save_int, save_chld;

	if (command == 0)
		return 1;

	save_quit = signal(SIGQUIT, SIG_IGN);
	save_int = signal(SIGINT, SIG_IGN);
	save_chld = signal(SIGCHLD, SIG_DFL);

	if ((pid = vfork()) < 0) {
		signal(SIGQUIT, save_quit);
		signal(SIGINT, save_int);
		signal(SIGCHLD, save_chld);
		return -1;
	}
	if (pid == 0) {
		signal(SIGQUIT, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		signal(SIGCHLD, SIG_DFL);

		execl("/bin/sh", "sh", "-c", command, (char *) 0);
		_exit(127);
	}
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT, SIG_IGN);

	while(true){
		ret = wait4(pid, &wait_val, WNOHANG, 0);
		if(ret == 0){
			if(s-- == 0){
				kill(pid, SIGTERM);
				printf("kill %d\n", pid);
				wait_val = -1;
				break;
			}
			usleep(1000000); // 1√Î
		}
		else if(ret == pid){
			break;
		}
		else{
			wait_val = -1;
			break;
		}
	}

	signal(SIGQUIT, save_quit);
	signal(SIGINT, save_int);
	signal(SIGCHLD, save_chld);
	return wait_val;
}

void system_result(const char *command, char& s)
{
	FILE * read_fp = popen(command, "r");
	if(read_fp)
	{
		while(true){
			char buffer[1024] = {0};
			int chars_read = fread(buffer, 1, sizeof(buffer), read_fp);
			if(chars_read <= 0)break;
			s.append(buffer, chars_read);
		}
	}
	pclose(read_fp);
}

FILE *fopen_exec(const char *command, const char *modes, pid_t& pid)
{
	FILE *fp;
	int pipe_fd[2];
	int parent_fd;
	int child_fd;
	int child_writing;		

	child_writing = 0;	
	if (modes[0] != 'w') {
		++child_writing;
		if (modes[0] != 'r') {
			return 0;
		}
	}

	if (pipe(pipe_fd)) {
		return 0;
	}

	child_fd = pipe_fd[child_writing];
	parent_fd = pipe_fd[1-child_writing];

	if (!(fp = fdopen(parent_fd, modes))) {
		close(parent_fd);
		close(child_fd);
		return 0;
	}

	if ((pid = vfork()) == 0) {
		close(parent_fd);
		if (child_fd != child_writing) 
			{
			dup2(child_fd, child_writing);
			close(child_fd);
		}

		execl("/bin/sh", "sh", "-c", command, (char *)0);

		_exit(127);
	}

	close(child_fd);

	if (pid > 0) {	
		return fp;
	}


	fclose(fp);	

	return 0;
}

int fclose_exec(FILE *stream, pid_t pid, int s)
{
	int wait_val, ret;

	fclose(stream);

	while(true){
		ret = wait4(pid, &wait_val, WNOHANG, 0);
		if(ret == 0){
			if(s-- == 0){
				kill(pid, SIGTERM);
				printf("kill %d\n", pid);
				wait_val = -1;
				break;
			}
			usleep(1000000); // 1√Î
		}
		else if(ret == pid){
			break;
		}
		else{
			wait_val = -1;
			break;
		}
	}

	return wait_val;
}

void zoom(byte * src_data, unsigned src_width, unsigned src_height, unsigned src_pitch, byte * dst_data, unsigned dst_width, unsigned dst_height, unsigned dst_pitch)
{
	unsigned* table = new unsigned[dst_width];
	for (unsigned x = 0; x < dst_width; ++x)
	{
		table[x] = (x * src_width / dst_width);
	}

	byte * dst = dst_data;
	for (unsigned y = 0; y < dst_height; ++y)
	{
		unsigned srcy = (y * src_height / dst_height);
		byte* src = src_data + src_pitch * srcy;
		for (unsigned x = 0; x < dst_width; ++x)
		{
			dst[x] = src[table[x]];
		}

		dst += dst_pitch;
	}

	delete [] table;
}

int bmp_write(const unsigned char *image, int xsize, int ysize, const char *filename)
{
    unsigned char header[54] = {
      0x42, 0x4d, 0, 0, 0, 0, 0, 0, 0, 0,
        54, 0, 0, 0, 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 24, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0
    };

    long file_size = (long)xsize * (long)ysize * 3 + 54;
    header[2] = (unsigned char)(file_size &0x000000ff);
    header[3] = (file_size >> 8) & 0x000000ff;
    header[4] = (file_size >> 16) & 0x000000ff;
    header[5] = (file_size >> 24) & 0x000000ff;

    long width = xsize;
    header[18] = width & 0x000000ff;
    header[19] = (width >> 8) &0x000000ff;
    header[20] = (width >> 16) &0x000000ff;
    header[21] = (width >> 24) &0x000000ff;

    long height = ysize;
    header[22] = height &0x000000ff;
    header[23] = (height >> 8) &0x000000ff;
    header[24] = (height >> 16) &0x000000ff;
    header[25] = (height >> 24) &0x000000ff;

    FILE *fp;
    if (!(fp = fopen(filename, "wb"))) 
      return -1;

    fwrite(header, sizeof(unsigned char), 54, fp);
    fwrite(image, sizeof(unsigned char), (size_t)(long)xsize * ysize * 3, fp);

    fclose(fp);
    return 0;
}

void flip_bmp(vector<byte>& b, int w, int h)
{
	vector<byte> t = b;
	for(size_t i = 0; i < b.size(); i += 3){
		b[i] = t[i + 2];
		b[i + 2] = t[i];
	}

	t = b;
	for(int i = 0; i < h; i++){
		byte * dst = &b[i * w * 3];
		byte * src = &t[(h - i - 1) * w * 3];
		memcpy(dst, src, w * 3);
	}
}

void rgb24_to_rgb1555(byte * rgb, int width, int height, byte * rgb1555, unsigned transparent)
{
	byte * crgb = (byte *)&transparent;
	byte cb = *crgb++;
	byte cg = *crgb++;
	byte cr = *crgb++;

	cb >>= 3;
	cg >>= 3;
	cr >>= 3;

	const unsigned short c1555 = (1 << 15) | (cb << 10) | (cg << 5) | cr;

	unsigned short * d1555 = (unsigned short *)rgb1555;

	for(int h = 0; h < height; ++h){
		for(int w = 0; w < width; ++w){
			byte b = *rgb; rgb++;
			byte g = *rgb; rgb++;
			byte r = *rgb; rgb++;

			b >>= 3;
			g >>= 3;
			r >>= 3;

			*d1555 = (1 << 15) | (b << 10) | (g << 5) | r;
			if(c1555 == *d1555){
				*d1555 = (b << 10) | (g << 5) | r;
			}
			d1555++;
		}
	}
}

void rgb24_to_rgb1555_flip(byte * rgb, int width, int height, byte * rgb1555, unsigned transparent)
{
	rgb += width * height * 3;
	rgb--;

	byte * crgb = (byte *)&transparent;

	byte cb = *crgb++;
	byte cg = *crgb++;
	byte cr = *crgb++;

	cb >>= 3;
	cg >>= 3;
	cr >>= 3;

	const unsigned short c1555 = (1 << 15) | (cb << 10) | (cg << 5) | cr;

	unsigned short * d1555 = (unsigned short *)rgb1555;

	for(int h = 0; h < height; ++h){
		for(int w = 0; w < width; ++w){
			byte r = *rgb; rgb--;
			byte g = *rgb; rgb--;
			byte b = *rgb; rgb--;

			b >>= 3;
			g >>= 3;
			r >>= 3;

			*d1555 = (1 << 15) | (b << 10) | (g << 5) | r;
			if(c1555 == *d1555){
				*d1555 = (b << 10) | (g << 5) | r;
			}
			d1555++;
		}
	}
}

*/

/*
void time_to_string_auto(char * buffer, time_t now)
{
    struct tm *p = localtime(&now);
    char buf[30];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    
    if(g_device_hardware == device_update::version::hi3511_tvp5150_v1 && g_vchn_attr[0].flip == 1)
    {
        int s = strlen(buf) - 1;
        for(int i = 0; i < s/2; ++i)
        {
            char t = buf[i];
            buf[i] = buf[s - i];
            buf[s - i] = t;
        }
    }
    strcpy(buffer, buf);
}
*/

void rgb24_to_rgb1555_flip(byte * rgb, int width, int height, byte * rgb1555, unsigned transparent)
{
    rgb += width * height * 3;
    rgb--;

    byte * crgb = (byte *)&transparent;

    byte cb = *crgb++;
    byte cg = *crgb++;
    byte cr = *crgb++;

    cb >>= 3;
    cg >>= 3;
    cr >>= 3;

    const unsigned short c1555 = (1 << 15) | (cb << 10) | (cg << 5) | cr;
    unsigned short * d1555 = (unsigned short *)rgb1555;

    int h;
    for(h = 0; h < height; ++h)
    {
        int w;
        for( w = 0; w < width; ++w)
        {
            byte r = *rgb; rgb--;
            byte g = *rgb; rgb--;
            byte b = *rgb; rgb--;

            b >>= 3;
            g >>= 3;
            r >>= 3;

            *d1555 = (1 << 15) | (b << 10) | (g << 5) | r;
            if(c1555 == *d1555)
            {
                *d1555 = (b << 10) | (g << 5) | r;
            }
            d1555++;
        }
    }
}

void rgb24_to_rgb1555_auto(byte * rgb, int width, int height, byte * rgb1555, unsigned transparent)
{
    byte * crgb = (byte *)&transparent;
    byte cb = *crgb++;
    byte cg = *crgb++;
    byte cr = *crgb++;

    cb >>= 3;
    cg >>= 3;
    cr >>= 3;

	const unsigned short c1555 = (1 << 15) | (cb << 10) | (cg << 5) | cr;

	unsigned short * d1555 = (unsigned short *)rgb1555;

    int h;
	for( h = 0; h < height; ++h)
    {
        int w;
		for( w = 0; w < width; ++w)
        {
			byte b = *rgb; rgb++;
			byte g = *rgb; rgb++;
			byte r = *rgb; rgb++;

			b >>= 3;
			g >>= 3;
			r >>= 3;

			*d1555 = (1 << 15) | (b << 10) | (g << 5) | r;
			if(c1555 == *d1555)
            {
				*d1555 = (b << 10) | (g << 5) | r;
			}
			d1555++;
		}
	}
}

void pcopy(byte * src, int src_width, int src_height, byte * dst, int dst_stride)
{
    int h;
	for( h = 0; h < src_height; ++h)
    {
        int w;
		for( w = 0; w < src_width; ++w)
        {
			*dst = *src;
			dst++;
			src++;
		}
		dst += dst_stride - src_width;
	}
}
