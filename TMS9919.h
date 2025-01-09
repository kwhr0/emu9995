class TMS9919 {
public:
	TMS9919();
	void Mute();
	void Set(int data);
	float Update();
private:
	int freq[4], cnt[4], v[4];
	float att[4];
	int rng, white, nf, osc, tmp;
};
