using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class bhColl : MonoBehaviour
{
    public GameObject golfBall;
    private Vector3 ballStartPos;
    public bool altControls = false;
    public bool hasJump=false;
    public float timeStart;
    // Start is called before the first frame update
    void Start()
    {
        ballStartPos = golfBall.transform.position;
    }

    // Update is called once per frame
    void Update()
    {
  
    }
    void resetGame()
    {        
        golfBall.GetComponent<Rigidbody>().velocity = new Vector3(0, 0, 0);
        golfBall.GetComponent<Rigidbody>().rotation = Quaternion.identity;
        golfBall.transform.position = ballStartPos;
        golfBall.transform.localScale = new Vector3(0.5f, 0.5f, 0.5f);
              
    }
    void OnCollisionEnter(Collision col)
    {
        Renderer rend = GetComponent<Renderer>();

        if (col.gameObject.tag.Equals("pole"))
        {
            resetGame();       
           
        }

        if (col.gameObject.tag.Equals("button"))
        {
            timeStart = Time.time;
            altControls = !altControls;
            rend.material.color = Color.green;
        }
        if (col.gameObject.tag.Equals("jumpButton"))
        {
            hasJump = true;
        }
        if (col.gameObject.tag.Equals("stickySnowball"))
        {
           
             golfBall.transform.localScale += new Vector3(0.2f, 0.2f, 0.2f);
             Destroy(col.gameObject);
        }
        if (col.gameObject.tag.Equals("killPlane"))
        {
            resetGame();
        }
    }

    void OnTriggerStay(Collider col)
    {
        if (col.gameObject.tag.Equals("water"))
        {
            golfBall.transform.localScale = Vector3.Lerp(golfBall.transform.localScale, new Vector3(0.5f, 0.5f, 0.5f),0.1f);
        }
    }
}
