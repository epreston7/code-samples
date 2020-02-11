using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class golfBallMovement : MonoBehaviour
{
   private float force;
   private bool altControls;
   private bool hasJump;
   private float timeStart = 0;
   public GameObject golfBall;

    Vector3 startPos;
    void Start()
    {
     
    }

    void jump()
    {
        if (Input.GetKeyDown(KeyCode.Space))
        {
            if (hasJump)
            {
                golfBall.GetComponent<Rigidbody>().AddForce(new Vector3(0, 600, 0));
                golfBall.GetComponent<bhColl>().hasJump = false;
                timeStart = 0;
            }
        }
    }

    // Update is called once per frame
    void Update()
    {
        float timeEnd;
        float timeHeld;

        Vector3 vel = golfBall.GetComponent<Rigidbody>().velocity;
        altControls = golfBall.GetComponent<bhColl>().altControls;
        hasJump = golfBall.GetComponent<bhColl>().hasJump;

        if (altControls)
        {
            timeHeld = Time.time - golfBall.GetComponent<bhColl>().timeStart;
            Debug.Log(timeHeld);
            if (timeHeld > 5)
            {
                golfBall.GetComponent<Renderer>().material.color = Color.white;
                golfBall.GetComponent<bhColl>().altControls = false;
                return;
            }

            jump();

            if (Input.GetKey(KeyCode.UpArrow))
            {
                golfBall.GetComponent<Rigidbody>().AddForce(new Vector3(0,0,1)*10);
            }
            if (Input.GetKey(KeyCode.DownArrow))
            {
                golfBall.GetComponent<Rigidbody>().AddForce(new Vector3(0, 0, -1) * 10);
            }
            if (Input.GetKey(KeyCode.RightArrow))
            {
                golfBall.GetComponent<Rigidbody>().AddForce(new Vector3(1, 0, 0) * 10);
            }
            if (Input.GetKey(KeyCode.LeftArrow))
            {
                golfBall.GetComponent<Rigidbody>().AddForce(new Vector3(-1, 0, 0) * 10);
            }
        }
        else
        {
            jump();

            if (vel.magnitude >= 0.3)
            {
                if (Physics.Raycast(golfBall.transform.position, new Vector3(0, -1, 0), 1))
                {
                    golfBall.transform.localScale += new Vector3(0.001f, 0.001f, 0.001f);
                }
            }

            if (vel.magnitude < 0.1)
            {
                

                if (Input.GetMouseButtonDown(0))
                {
                    timeStart = Time.time;
                }

                

                if (Input.GetMouseButtonUp(0))
                {
                    timeEnd = Time.time;
                    timeHeld = timeEnd - timeStart;
                    force = 800 * timeHeld;

                    if (force > 2000)
                    {
                        force = 2000;
                    }
                    Debug.Log(force);
                    timeStart = 0;
                    RaycastHit hit;

                    if (Physics.Raycast(Camera.main.ScreenPointToRay(Input.mousePosition), out hit, 100))
                    {
                        Vector3 forceDir = hit.point - transform.position;
                        GetComponent<Rigidbody>().AddForce((forceDir.normalized) * force);
                    }
                }

            }
        }
    }
}
