class DestructTracker
{
  public:
  DestructTracker(bool& d)
  : m_d(d)
  {
  }

  ~DestructTracker() 
  {
    m_d = true;
  }

  private:
  bool& m_d;
};
